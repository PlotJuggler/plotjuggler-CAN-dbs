#pragma once

#include <QObject>
#include <QtPlugin>
#include <PlotJuggler/dataloader_base.h>
#include "../PluginsCommonCAN/CanFrameProcessor.h"

using namespace PJ;

class DataLoadCAN : public DataLoader
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataLoader")
  Q_INTERFACES(PJ::DataLoader)

public:
  DataLoadCAN();
  virtual const std::vector<const char *> &compatibleFileExtensions() const override;
  virtual QSize inspectFile(QFile *file);
  virtual bool readDataFromFile(FileLoadInfo *fileload_info, PlotDataMapRef &plot_data_map) override;
  bool loadCANDatabase(PlotDataMapRef &plot_data_map, std::string dbc_file_location, CanFrameProcessor::CanProtocol protocol);
  virtual ~DataLoadCAN();

  virtual const char *name() const override
  {
    return "DataLoad CAN";
  }

  virtual bool xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const override;
  virtual bool xmlLoadState(const QDomElement &parent_element) override;

private:
  std::vector<const char *> extensions_;
  std::string default_time_axis_;
  std::unique_ptr<CanFrameProcessor> frame_processor_;
};
