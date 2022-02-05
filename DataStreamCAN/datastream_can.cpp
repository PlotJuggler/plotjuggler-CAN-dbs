#include "datastream_can.h"
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
#include <dbcppp/Network.h>

using namespace PJ;

DataStreamCAN::DataStreamCAN() : connect_dialog_{new ConnectDialog()}
{
  connect(connect_dialog_, &QDialog::accepted, this, &DataStreamCAN::connectCanInterface);
}

void DataStreamCAN::connectCanInterface()
{
  const ConnectDialog::Settings p = connect_dialog_->settings();

  QString errorString;
  can_interface_ = QCanBus::instance()->createDevice(p.backendName, p.deviceInterfaceName,
                                                     &errorString);
  if (!can_interface_)
  {
    qDebug() << tr("Error creating device '%1', reason: '%2'")
                    .arg(p.backendName)
                    .arg(errorString);
    return;
  }

  if (p.useConfigurationEnabled)
  {
    for (const ConnectDialog::ConfigurationItem &item : p.configurations)
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
    std::ifstream dbc_file{p.canDatabaseLocation.toStdString()};
    can_network_ = dbcppp::INetwork::LoadDBCFromIs(dbc_file);
    messages_.clear();
    for (const dbcppp::IMessage& msg : can_network_->Messages())
    {
      messages_.insert(std::make_pair(msg.Id(), &msg));
    }

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
      qDebug() << tr("Backend: %1, connected to %2")
                      .arg(p.backendName)
                      .arg(p.deviceInterfaceName);
    }
  }
}

bool DataStreamCAN::start(QStringList *)
{
  connect_dialog_->show();
  thread_ = std::thread([this]()
                        { this->loop(); });
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

bool DataStreamCAN::xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const
{
  return true;
}

bool DataStreamCAN::xmlLoadState(const QDomElement &parent_element)
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
    if (can_network_)
    {
      double now = frame.timeStamp().seconds() + frame.timeStamp().microSeconds() * 1e-6;
      auto messages_iter = messages_.find(frame.frameId());
      if (messages_iter != messages_.end())
      {
        const dbcppp::IMessage* msg = messages_iter->second;
        for (const dbcppp::ISignal& signal : msg->Signals())
        {
          double decoded_val = signal.RawToPhys(signal.Decode(frame.payload().data()));
          auto str = QString("can_frames/%1/").arg(msg->Id()).toStdString() + signal.Name();
          //qCritical() << str.c_str();
          auto it = dataMap().numeric.find(str);
          if (it != dataMap().numeric.end())
          {
            auto &plot = it->second;
            plot.pushBack({now, decoded_val});
          }
        }
      }
    }
  }
}

void DataStreamCAN::loop()
{
  // Block until both are initalized
  while (can_interface_ == nullptr || can_network_ == nullptr)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  // Add all signals by name
  {
    std::lock_guard<std::mutex> lock(mutex());
    // Add all signals by name
    for(auto it = messages_.begin(); it != messages_.end(); it++)
    {
      const dbcppp::IMessage* msg = it->second;
      for (const dbcppp::ISignal& signal : msg->Signals())
      {
        auto str = QString("can_frames/%1/").arg(msg->Id()).toStdString() + signal.Name();
        auto it = dataMap().addNumeric(str);
        auto &plot = it->second;
        plot.pushBack(PlotData::Point(0, 0)); // if not pushed once, data is not visible in PJ, don't know why.
      }
    }
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
