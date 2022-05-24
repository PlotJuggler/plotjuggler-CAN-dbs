#include "dataload_can.h"
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

// Regular expression for log files created by candump -L
// Captured groups: time, channel, frame_id, payload
const QRegularExpression canlog_rgx("\\((\\d*\\.\\d*)\\)\\s*([\\S]*)\\s*([0-9a-fA-F]{3,8})\\#([0-9a-fA-F]*)");

DataLoadCAN::DataLoadCAN()
{
  extensions_.push_back("log");
}

const std::vector<const char *> &DataLoadCAN::compatibleFileExtensions() const
{
  return extensions_;
}

bool DataLoadCAN::loadCANDatabase(QString dbc_filename)
{
  // Get dbc file and add frames to dataMap()
  auto dbc_dialog = QFileDialog::getOpenFileUrl(
    Q_NULLPTR,
    tr("Select CAN database"),
    QUrl(),tr("CAN database (*.dbc)")).toLocalFile();
  std::ifstream dbc_file{dbc_dialog.toStdString()};
  can_network_ = dbcppp::INetwork::LoadDBCFromIs(dbc_file);
  messages_.clear();
  for (const dbcppp::IMessage& msg : can_network_->Messages())
  {
    messages_.insert(std::make_pair(msg.Id(), &msg));
  }
}

QSize DataLoadCAN::inspectFile(QFile *file)
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

bool DataLoadCAN::readDataFromFile(FileLoadInfo *info, PlotDataMapRef &plot_data)
{
  bool use_provided_configuration = false;

  if (info->plugin_config.hasChildNodes())
  {
    use_provided_configuration = true;
    xmlLoadState(info->plugin_config.firstChildElement());
  }

  const int TIME_INDEX_NOT_DEFINED = -2;

  int time_index = TIME_INDEX_NOT_DEFINED;

  QFile file(info->filename);
  file.open(QFile::ReadOnly);

  std::vector<std::string> column_names;

  const QSize table_size = inspectFile(&file);
  const int tot_lines = table_size.height() - 1;
  const int columncount = table_size.width();
  file.close();

  loadCANDatabase("FilenameNotUsed");
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

  // Add all signals by name
  for(auto it = messages_.begin(); it != messages_.end(); it++)
  {
    const dbcppp::IMessage* msg = it->second;
    for (const dbcppp::ISignal& signal : msg->Signals())
    {
      auto str = QString("can_frames/%1/").arg(msg->Id()).toStdString() + signal.Name();
      plot_data.addNumeric(str);
    }
  }

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
      continue; // skip invalid lines
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
    auto messages_iter = messages_.find(frameId);
    if (messages_iter != messages_.end())
    {
      const dbcppp::IMessage* msg = messages_iter->second;
      for (const dbcppp::ISignal& signal : msg->Signals())
      {
        double decoded_val = signal.RawToPhys(signal.Decode(frameDataBytes));
        auto str = QString("can_frames/%1/").arg(frameId).toStdString() + signal.Name();
        auto it = plot_data.numeric.find(str);
        if (it != plot_data.numeric.end())
        {
          auto &plot = it->second;
          plot.pushBack(PlotData::Point(frameTime, decoded_val));
        }
      }
    }
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
    plot_data.numeric.clear();
  }

  if (monotonic_warning)
  {
    QString message = "Two consecutive samples had the same X value (i.e. time).\n"
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

bool DataLoadCAN::xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const
{
  QDomElement elem = doc.createElement("default");
  elem.setAttribute("time_axis", default_time_axis_.c_str());

  parent_element.appendChild(elem);
  return true;
}

bool DataLoadCAN::xmlLoadState(const QDomElement &parent_element)
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
