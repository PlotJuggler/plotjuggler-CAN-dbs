#include <QTextStream>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QCanBus>
#include <QCanBusFrame>

#include <thread>
#include <mutex>
#include <chrono>
#include <thread>
#include <fstream>

#include "datastream_can.h"

using namespace PJ;

DataStreamCAN::DataStreamCAN() : connect_dialog_{ new ConnectDialog() }
{
  connect(connect_dialog_, &QDialog::accepted, this, &DataStreamCAN::connectCanInterface);
}

void DataStreamCAN::connectCanInterface()
{
  const ConnectDialog::Settings p = connect_dialog_->settings();

  QString errorString;
  can_interface_ = QCanBus::instance()->createDevice(p.backendName, p.deviceInterfaceName, &errorString);
  if (!can_interface_)
  {
    qDebug() << tr("Error creating device '%1', reason: '%2'").arg(p.backendName).arg(errorString);
    return;
  }

  if (p.useConfigurationEnabled)
  {
    for (const ConnectDialog::ConfigurationItem& item : p.configurations)
      can_interface_->setConfigurationParameter(item.first, item.second);
  }

  if (!can_interface_->connectDevice())
  {
    qDebug() << tr("Connection error: %1").arg(can_interface_->errorString());

    delete can_interface_;
    can_interface_ = nullptr;
  }
  else
  {
    std::ifstream dbc_file{ p.canDatabaseLocation.toStdString() };
    frame_processor_ = std::make_unique<CanFrameProcessor>(dbc_file, dataMap(), p.protocol);

    QVariant bitRate = can_interface_->configurationParameter(QCanBusDevice::BitRateKey);
    if (bitRate.isValid())
    {
      qDebug() << tr("Backend: %1, connected to %2 at %3 kBit/s")
                      .arg(p.backendName)
                      .arg(p.deviceInterfaceName)
                      .arg(bitRate.toInt() / 1000);
    }
    else
    {
      qDebug() << tr("Backend: %1, connected to %2").arg(p.backendName).arg(p.deviceInterfaceName);
    }
  }
}

bool DataStreamCAN::start(QStringList*)
{
  if (running_) {
    return running_;
  }
  connect_dialog_->show();
  int res = connect_dialog_->exec();
  if (res != QDialog::Accepted)
  {
    return false;
  }
  thread_ = std::thread([this]() { this->loop(); });
  return true;
}

void DataStreamCAN::shutdown()
{
  running_ = false;
  if (thread_.joinable())
    thread_.join();
}

bool DataStreamCAN::isRunning() const
{
  return running_;
}

DataStreamCAN::~DataStreamCAN()
{
  shutdown();
}

bool DataStreamCAN::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  return true;
}

bool DataStreamCAN::xmlLoadState(const QDomElement& parent_element)
{
  return true;
}

void DataStreamCAN::pushSingleCycle()
{
  std::lock_guard<std::mutex> lock(mutex());

  // Since readAllFrames is introduced in Qt5.12, reading using for
  auto n_frames = can_interface_->framesAvailable();
  for (int i = 0; i < n_frames; i++)
  {
    auto frame = can_interface_->readFrame();
    double timestamp = frame.timeStamp().seconds() + frame.timeStamp().microSeconds() * 1e-6;
    frame_processor_->ProcessCanFrame(frame.frameId(), (const uint8_t*)frame.payload().data(), 8, timestamp);
  }
}

void DataStreamCAN::loop()
{
  // Block until both are initalized
  while (can_interface_ == nullptr || frame_processor_ == nullptr)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  running_ = true;
  while (running_)
  {
    auto prev = std::chrono::high_resolution_clock::now();
    pushSingleCycle();
    emit dataReceived();
    std::this_thread::sleep_until(prev + std::chrono::milliseconds(20));
  }
}
