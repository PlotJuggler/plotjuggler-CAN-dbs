#ifndef DIALOG_SELECT_CAN_DATABASE_H
#define DIALOG_SELECT_CAN_DATABASE_H

#include <QDialog>
#include <QString>
#include <QFile>
#include <QStringList>
#include <QCheckBox>
#include <QShortcut>
#include <QDomDocument>
#include "../PluginsCommonCAN/CanFrameProcessor.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class DialogSelectCanDatabase;
}
QT_END_NAMESPACE


class DialogSelectCanDatabase : public QDialog
{
  Q_OBJECT

public:
  explicit DialogSelectCanDatabase(QWidget* parent = nullptr);
  QString GetDatabaseLocation() const;
  CanFrameProcessor::CanProtocol GetCanProtocol() const;

  ~DialogSelectCanDatabase() override;

private slots:
  void Ok();
  void Cancel();

private:
  Ui::DialogSelectCanDatabase* ui_;
  QString database_location_;
  CanFrameProcessor::CanProtocol protocol_;

  void ImportDatabaseLocation();
};

#endif  // DIALOG_SELECT_CAN_DATABASE_H
