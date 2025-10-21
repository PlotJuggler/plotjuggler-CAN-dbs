#ifndef DIALOG_SELECT_CAN_DATABASE_H
#define DIALOG_SELECT_CAN_DATABASE_H

#include <QCheckBox>
#include <QDialog>
#include <QDomDocument>
#include <QFile>
#include <QShortcut>
#include <QString>
#include <QStringList>
#include "../PluginsCommonCAN/CanFrameProcessor.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class DialogSelectCanDatabase;
}
QT_END_NAMESPACE

class DialogSelectCanDatabase : public QDialog {
  Q_OBJECT

public:
  explicit DialogSelectCanDatabase(const QStringList& existing_files = QStringList(), QWidget* parent = nullptr);
  QStringList GetDatabaseLocations() const;
  CanFrameProcessor::CanProtocol GetCanProtocol() const;
  bool UseEnhancedMetadata() const;

  ~DialogSelectCanDatabase() override;

private slots:
  void Ok();
  void Cancel();
  void AddDatabaseFile();
  void RemoveSelectedDatabaseFile();
  void UpdateButtonStates();

private:
  Ui::DialogSelectCanDatabase* ui_;
  QStringList database_locations_;
  CanFrameProcessor::CanProtocol protocol_;
  bool use_enhanced_metadata_ = true;
};

#endif  // DIALOG_SELECT_CAN_DATABASE_H
