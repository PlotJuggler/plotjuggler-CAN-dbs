#pragma once

#include <QtPlugin>
#include <QCanBus>
#include <QCanBusFrame>
#include <thread>

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
  bool start(QStringList*) override;
  void shutdown() override;
  bool isRunning() const override;
  ~DataStreamCAN() override;

  const char* name() const override
  {
    return "CAN Streamer";
  }

  bool isDebugPlugin() override
  {
    return false;
  }

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;
  bool xmlLoadState(const QDomElement& parent_element) override;

private slots:
  void connectCanInterface();

private:
  ConnectDialog *connect_dialog_;
  QCanBusDevice *can_interface_ = nullptr;
  std::unique_ptr<CanFrameProcessor> frame_processor_;

  std::thread thread_;
  bool running_;
  void loop();
  void pushSingleCycle();
};

