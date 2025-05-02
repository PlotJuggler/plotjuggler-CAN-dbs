/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the QtSerialBus module.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDebug>
#include <QCanBus>
#include <QCanBusFrame>
#include <QDomDocument>
#include <QSettings>
#include <fstream>
#include <vector>

#include "datastream_can.h"
#include "../PluginsCommonCAN/select_can_database.h"

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

  // Only open dialog if no database locations are configured
  QStringList database_locations = p.canDatabaseLocations;
  CanFrameProcessor::CanProtocol protocol = p.protocol;
  bool use_enhanced_metadata = p.use_enhanced_metadata;

  if (database_locations.isEmpty())
  {
    DialogSelectCanDatabase *dialog = new DialogSelectCanDatabase();
    if (dialog->exec() != static_cast<int>(QDialog::Accepted))
    {
      delete dialog;
      can_interface_.reset();
      return;
    }

    database_locations = dialog->GetDatabaseLocations();
    protocol = dialog->GetCanProtocol();
    use_enhanced_metadata = dialog->UseEnhancedMetadata();

    connect_dialog_->setDatabaseSettings(database_locations, protocol, use_enhanced_metadata);

    delete dialog;
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
  for (const QString &location : database_locations)
  {
    dbc_files.emplace_back(location.toStdString());
  }

  frame_processor_ = std::make_unique<CanFrameProcessor>(dbc_files, dataMap(), protocol);

  // Configure enhanced metadata
  if (frame_processor_)
  {
    frame_processor_->setUseEnhancedMetadata(use_enhanced_metadata);
  }

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
