#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QProgressDialog>
#include <QFileDialog>
#include <QRegularExpression>

#include <fstream>
#include <cstring>
#include <clocale>
#include "dataload_can.h"
#include "../PluginsCommonCAN/select_can_database.h"

// Regular expression for log files created by candump -L
// Captured groups: time, channel, frame_id, payload
const QRegularExpression canlog_rgx("\\((\\d*\\.\\d*)\\)\\s*([\\S]*)\\s*([0-9a-fA-F]{3,8})\\#([0-9a-fA-F]*)");

DataLoadCAN::DataLoadCAN()
{
  extensions_.push_back("log");
}

const std::vector<const char*>& DataLoadCAN::compatibleFileExtensions() const
{
  return extensions_;
}

bool DataLoadCAN::loadCANDatabase(PlotDataMapRef& plot_data_map, std::string dbc_file_location,
                                  CanFrameProcessor::CanProtocol protocol)
{
  std::ifstream dbc_file{ dbc_file_location };
  frame_processor_ = std::make_unique<CanFrameProcessor>(dbc_file, plot_data_map, protocol);
  return true;
}

QSize DataLoadCAN::inspectFile(QFile* file)
{
  QTextStream inA(file);
  int linecount = 0;

  while (!inA.atEnd())
  {
    inA.readLine();
    linecount++;
  }

  QSize table_size;
  table_size.setWidth(4);
  table_size.setHeight(linecount);

  return table_size;
}

bool DataLoadCAN::readDataFromFile(FileLoadInfo* fileload_info, PlotDataMapRef& plot_data_map)
{
  bool use_provided_configuration = false;

  if (fileload_info->plugin_config.hasChildNodes())
  {
    use_provided_configuration = true;
    xmlLoadState(fileload_info->plugin_config.firstChildElement());
  }

  const int TIME_INDEX_NOT_DEFINED = -2;

  int time_index = TIME_INDEX_NOT_DEFINED;

  QFile file(fileload_info->filename);
  file.open(QFile::ReadOnly);

  std::vector<std::string> column_names;

  const QSize table_size = inspectFile(&file);
  const int tot_lines = table_size.height() - 1;
  const int columncount = table_size.width();
  file.close();

  DialogSelectCanDatabase* dialog = new DialogSelectCanDatabase();

  if (dialog->exec() != static_cast<int>(QDialog::Accepted))
  {
    return false;
  }
  loadCANDatabase(plot_data_map, dialog->GetDatabaseLocation().toStdString(), dialog->GetCanProtocol());

  file.open(QFile::ReadOnly);
  QTextStream inB(&file);

  bool interrupted = false;

  int linecount = 0;

  QProgressDialog progress_dialog;
  progress_dialog.setLabelText("Loading... please wait");
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.setRange(0, tot_lines);
  progress_dialog.setAutoClose(true);
  progress_dialog.setAutoReset(true);
  progress_dialog.show();

  bool monotonic_warning = false;
  // To have . as decimal seperator, save current locale and change it.
  const auto oldLocale = std::setlocale(LC_NUMERIC, nullptr);
  std::setlocale(LC_NUMERIC, "C");
  while (!inB.atEnd())
  {
    QString line = inB.readLine();
    static QRegularExpressionMatchIterator rxIterator;
    rxIterator = canlog_rgx.globalMatch(line);
    if (!rxIterator.hasNext())
    {
      continue;  // skip invalid lines
    }
    QRegularExpressionMatch canFrame = rxIterator.next();
    uint64_t frameId = std::stoul(canFrame.captured(3).toStdString(), 0, 16);
    double frameTime = std::stod(canFrame.captured(1).toStdString());

    int dlc = canFrame.capturedLength(4) / 2;
    std::string frameDataString;
    // When dlc is less than 8, right padding is required
    if (dlc < 8)
    {
      std::string padding = std::string(2 * (8 - dlc), '0');
      frameDataString = canFrame.captured(4).toStdString().append(padding);
    }
    else
    {
      frameDataString = canFrame.captured(4).toStdString();
    }

    uint64_t frameData = std::stoul(frameDataString, 0, 16);
    uint8_t frameDataBytes[8];
    std::memcpy(frameDataBytes, &frameData, 8);
    std::reverse(frameDataBytes, frameDataBytes + 8);
    frame_processor_->ProcessCanFrame(frameId, frameDataBytes, 8, frameTime);
    //------ progress dialog --------------
    if (linecount++ % 100 == 0)
    {
      progress_dialog.setValue(linecount);
      QApplication::processEvents();

      if (progress_dialog.wasCanceled())
      {
        return false;
      }
    }
  }
  // Restore locale setting
  std::setlocale(LC_NUMERIC, oldLocale);
  file.close();

  if (interrupted)
  {
    progress_dialog.cancel();
    plot_data_map.numeric.clear();
  }

  if (monotonic_warning)
  {
    QString message =
        "Two consecutive samples had the same X value (i.e. time).\n"
        "Since PlotJuggler makes the assumption that timeseries are strictly monotonic, you "
        "might experience undefined behaviours.\n\n"
        "You have been warned...";
    QMessageBox::warning(0, tr("Warning"), message);
  }

  return true;
}

DataLoadCAN::~DataLoadCAN()
{
}

bool DataLoadCAN::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  QDomElement elem = doc.createElement("default");
  elem.setAttribute("time_axis", default_time_axis_.c_str());

  parent_element.appendChild(elem);
  return true;
}

bool DataLoadCAN::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement elem = parent_element.firstChildElement("default");
  if (!elem.isNull())
  {
    if (elem.hasAttribute("time_axis"))
    {
      default_time_axis_ = elem.attribute("time_axis").toStdString();
      return true;
    }
  }
  return false;
}
