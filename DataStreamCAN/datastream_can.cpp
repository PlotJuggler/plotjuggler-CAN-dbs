#include <QDebug>
#include <QCanBus>
#include <QCanBusFrame>
#include <QDomDocument>
#include <fstream>
#include <vector>

#include "datastream_can.h"

using namespace PJ;

DataStreamCAN::DataStreamCAN()
    : connect_dialog_(std::make_unique<ConnectDialog>()),
      running_(false)
{
  connect(connect_dialog_.get(), &QDialog::accepted, this, &DataStreamCAN::connectCanInterface);
}

DataStreamCAN::~DataStreamCAN()
{
  shutdown();
}

void DataStreamCAN::connectCanInterface()
{
  const ConnectDialog::Settings p = connect_dialog_->settings();

  QString errorString;
  can_interface_.reset(QCanBus::instance()->createDevice(p.backendName, p.deviceInterfaceName, &errorString));

  if (!can_interface_)
  {
    qWarning() << tr("Error creating device '%1', reason: '%2'")
                      .arg(p.backendName)
                      .arg(errorString);
    return;
  }

  if (p.useConfigurationEnabled)
  {
    for (const ConnectDialog::ConfigurationItem &item : p.configurations)
      can_interface_->setConfigurationParameter(item.first, item.second);
  }

  // Connect signals before connecting the device
  connect(can_interface_.get(), &QCanBusDevice::framesReceived,
          this, &DataStreamCAN::processReceivedFrames);
  connect(can_interface_.get(), &QCanBusDevice::errorOccurred,
          this, &DataStreamCAN::handleCanError);

  if (!can_interface_->connectDevice())
  {
    qWarning() << tr("Connection error: %1").arg(can_interface_->errorString());
    can_interface_.reset();
    return;
  }

  // Load multiple DBC files
  std::vector<std::ifstream> dbc_files;
  for (const QString &location : p.canDatabaseLocations)
  {
    dbc_files.emplace_back(location.toStdString());
  }

  frame_processor_ = std::make_unique<CanFrameProcessor>(dbc_files, dataMap(), p.protocol);

  QVariant bitRate = can_interface_->configurationParameter(QCanBusDevice::BitRateKey);
  if (bitRate.isValid())
  {
    qInfo() << tr("Backend: %1, connected to %2 at %3 kBit/s")
                   .arg(p.backendName)
                   .arg(p.deviceInterfaceName)
                   .arg(bitRate.toInt() / 1000);
  }
  else
  {
    qInfo() << tr("Backend: %1, connected to %2")
                   .arg(p.backendName)
                   .arg(p.deviceInterfaceName);
  }
}

bool DataStreamCAN::start(QStringList *pre_selected_sources)
{
  if (running_)
  {
    return true;
  }

  if (!connect_dialog_->exec() || connect_dialog_->result() != QDialog::Accepted)
  {
    return false;
  }

  // Check interface and frame processor were properly initialized
  if (!can_interface_ || !frame_processor_)
  {
    qWarning() << "Failed to initialize CAN interface or frame processor";
    return false;
  }

  running_ = true;
  return true;
}

void DataStreamCAN::shutdown()
{
  if (!running_)
    return;

  running_ = false;

  // Clean up resources - disconnect signals first
  if (can_interface_)
  {
    disconnect(can_interface_.get(), &QCanBusDevice::framesReceived,
               this, &DataStreamCAN::processReceivedFrames);
    disconnect(can_interface_.get(), &QCanBusDevice::errorOccurred,
               this, &DataStreamCAN::handleCanError);

    if (can_interface_->state() == QCanBusDevice::ConnectedState)
    {
      can_interface_->disconnectDevice();
    }
  }

  can_interface_.reset();
  frame_processor_.reset();
}

bool DataStreamCAN::isRunning() const
{
  return running_;
}

void DataStreamCAN::processReceivedFrames()
{
  if (!can_interface_ || !frame_processor_ || !running_)
    return;

  std::lock_guard<std::mutex> lock(mutex());

  // Use readAllFrames which is supported in all Qt versions
  const QVector<QCanBusFrame> frames = can_interface_->readAllFrames();

  if (frames.isEmpty())
    return;

  for (const QCanBusFrame &frame : frames)
  {
    try
    {
      double timestamp = frame.timeStamp().seconds() + frame.timeStamp().microSeconds() * 1e-6;
      frame_processor_->ProcessCanFrame(
          frame.frameId(),
          reinterpret_cast<const uint8_t *>(frame.payload().data()),
          frame.payload().size(),
          timestamp);
    }
    catch (const std::exception &e)
    {
      qWarning() << "Exception processing CAN frame:" << e.what();
    }
    catch (...)
    {
      qWarning() << "Unknown exception processing CAN frame";
    }
  }

  // Notify that new data is available
  emit dataReceived();
}

void DataStreamCAN::handleCanError(QCanBusDevice::CanBusError error)
{
  if (!can_interface_)
    return;

  QString errorString;
  switch (error)
  {
  case QCanBusDevice::ReadError:
    errorString = "Read error";
    break;
  case QCanBusDevice::WriteError:
    errorString = "Write error";
    break;
  case QCanBusDevice::ConnectionError:
    errorString = "Connection error";
    break;
  case QCanBusDevice::ConfigurationError:
    errorString = "Configuration error";
    break;
  case QCanBusDevice::OperationError:
    errorString = "Operation error";
    break;
  case QCanBusDevice::TimeoutError:
    errorString = "Timeout error";
    break;
  case QCanBusDevice::UnknownError:
    errorString = "Unknown error";
    break;
  default:
    errorString = "Error";
    break;
  }

  qWarning() << "CAN bus error:" << errorString << "-" << can_interface_->errorString();

  // For serious errors, shutdown the connection
  if (error == QCanBusDevice::ConnectionError)
  {
    shutdown();
  }
}

bool DataStreamCAN::xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const
{
  return true;
}

bool DataStreamCAN::xmlLoadState(const QDomElement &parent_element)
{
  return true;
}
