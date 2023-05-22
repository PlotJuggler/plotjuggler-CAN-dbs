#include <QFileDialog>

#include "select_can_database.h"
#include "ui_select_can_database.h"

DialogSelectCanDatabase::DialogSelectCanDatabase(QWidget* parent)
  : QDialog(parent), ui_(new Ui::DialogSelectCanDatabase), database_location_{}, protocol_{}
{
  ui_->setupUi(this);
  ui_->protocolListBox->addItem(tr("RAW"), QVariant(true));
  ui_->protocolListBox->addItem(tr("NMEA2K"), QVariant(true));
  ui_->protocolListBox->addItem(tr("J1939"), QVariant(true));
  ui_->protocolListBox->setCurrentIndex(0);
  ui_->okButton->setEnabled(false);

  connect(ui_->okButton, &QPushButton::clicked, this, &DialogSelectCanDatabase::Ok);
  connect(ui_->cancelButton, &QPushButton::clicked, this, &DialogSelectCanDatabase::Cancel);
  connect(ui_->loadDatabaseButton, &QPushButton::clicked, this, &DialogSelectCanDatabase::ImportDatabaseLocation);
}
QString DialogSelectCanDatabase::GetDatabaseLocation() const
{
  return database_location_;
}
CanFrameProcessor::CanProtocol DialogSelectCanDatabase::GetCanProtocol() const
{
  return protocol_;
}

DialogSelectCanDatabase::~DialogSelectCanDatabase()
{
  delete ui_;
}

void DialogSelectCanDatabase::Ok()
{
  auto protocol_text = ui_->protocolListBox->currentText().toStdString();
  if (protocol_text == "RAW")
  {
    protocol_ = CanFrameProcessor::CanProtocol::RAW;
  }
  else if (protocol_text == "NMEA2K")
  {
    protocol_ = CanFrameProcessor::CanProtocol::NMEA2K;
  }
  else if (protocol_text == "J1939")
  {
    protocol_ = CanFrameProcessor::CanProtocol::J1939;
  }
  accept();
}

void DialogSelectCanDatabase::Cancel()
{
  reject();
}

void DialogSelectCanDatabase::ImportDatabaseLocation()
{
  database_location_ =
      QFileDialog::getOpenFileUrl(Q_NULLPTR, tr("Select CAN database"), QUrl(), tr("CAN database (*.dbc)"))
          .toLocalFile();
  // Since file is gotten, enable ok button.
  ui_->okButton->setEnabled(true);
}
