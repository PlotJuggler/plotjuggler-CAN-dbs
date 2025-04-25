#pragma once

#include <QtPlugin>
#include <QCanBus>
#include <QCanBusFrame>
#include <memory>

#include <PlotJuggler/datastreamer_base.h>

#include "connectdialog.h"
#include "../PluginsCommonCAN/CanFrameProcessor.h"

class DataStreamCAN : public PJ::DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
  Q_INTERFACES(PJ::DataStreamer)

public:
  DataStreamCAN();
  ~DataStreamCAN() override;

  bool start(QStringList *pre_selected_sources = nullptr) override;
  void shutdown() override;
  bool isRunning() const override;

  const char *name() const override
  {
    return "CAN Streamer";
  }

  bool isDebugPlugin() override
  {
    return false;
  }

  bool xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const override;
  bool xmlLoadState(const QDomElement &parent_element) override;

private slots:
  void connectCanInterface();
  void processReceivedFrames();
  void handleCanError(QCanBusDevice::CanBusError error);

private:
  std::unique_ptr<ConnectDialog> connect_dialog_;
  std::unique_ptr<QCanBusDevice> can_interface_;
  std::unique_ptr<CanFrameProcessor> frame_processor_;
  bool running_;
};
