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
#include <math.h>

#include <fstream>
#include <dbcppp/Network.h>

using namespace PJ;

DataStreamCAN::DataStreamCAN()
{

}

bool DataStreamCAN::start(QStringList*)
{
  _running = true;

  // Get dbc file
  auto dbc_dialog = QFileDialog::getOpenFileUrl().toLocalFile();
  std::ifstream dbc_file{ dbc_dialog.toStdString() };
  _can_network = dbcppp::Network::loadDBCFromIs(dbc_file);
  
  // Add all signals by name
  _can_network->forEachMessage([&](const dbcppp::Message &msg) {
    msg.forEachSignal([&](const dbcppp::Signal &signal) {
      auto str = QString("can_frames/%1/").arg(msg.getId()).toStdString() + signal.getName();
      auto it = dataMap().addNumeric(str);
      auto& plot = it->second;
      plot.pushBack(PlotData::Point(0, 0));  // if not pushed once, data is not visible in PJ, don't know why.
    });
  });
  
  // Create can device and connect to it, this needs to be handled in ui
  QString errorString;
  _can_device = QCanBus::instance()->createDevice(QStringLiteral("socketcan"), QStringLiteral("vcan0"), &errorString);
  
  if (_can_device != nullptr)
  {
    qDebug() << "Created device, state is:" << _can_device->state();
  }
  else
  {
    qFatal("Unable to create CAN device.");
  }
  if (_can_device->connectDevice())
  {
    qDebug() << "Connected, state is:" << _can_device->state();
  }
  else
  {
    qDebug() << "Connect failed, error is:" << _can_device->errorString();
  }

  _thread = std::thread([this]() { this->loop(); });
  return true;
}

void DataStreamCAN::shutdown()
{
  _running = false;
  if (_thread.joinable())
    _thread.join();
}

bool DataStreamCAN::isRunning() const
{
  return _running;
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
  auto n_frames = _can_device->framesAvailable();
  for (int i = 0; i < n_frames; i++)
  {
    auto frame = _can_device->readFrame();
    if (_can_network)
    {
      double now = frame.timeStamp().seconds() + frame.timeStamp().microSeconds() * 1e-6;
      const dbcppp::Message* msg = _can_network->getMessageById(frame.frameId());
      if (msg)
      {
        msg->forEachSignal([&](const dbcppp::Signal& signal) {
          double decoded_val = signal.rawToPhys(signal.decode(frame.payload().data()));
          auto str = QString("can_frames/%1/").arg(msg->getId()).toStdString() + signal.getName();
          //qCritical() << str.c_str();
          auto it = dataMap().numeric.find(str);
          if (it != dataMap().numeric.end())
          {
            auto& plot = it->second;
            plot.pushBack({now, decoded_val});
          }
          });
      }
    }
  }
}

void DataStreamCAN::loop()
{
  _running = true;
  while (_running)
  {
    auto prev = std::chrono::high_resolution_clock::now();
    pushSingleCycle();
    emit dataReceived();
    std::this_thread::sleep_until(prev + std::chrono::milliseconds(20));
  }
}
