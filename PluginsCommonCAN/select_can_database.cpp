#include <QFileDialog>
#include <QSettings>

#include "select_can_database.h"
#include "ui_select_can_database.h"

// Public
DialogSelectCanDatabase::DialogSelectCanDatabase(const QStringList& existing_files, QWidget* parent)
  : QDialog(parent)
  , ui_(new Ui::DialogSelectCanDatabase)
  , database_locations_(existing_files)
  , protocol_(CanFrameProcessor::RAW)
  , use_enhanced_metadata_(true) {
  ui_->setupUi(this);
  ui_->protocolListBox->addItem(tr("RAW"), QVariant(true));
  ui_->protocolListBox->addItem(tr("NMEA2K"), QVariant(true));
  ui_->protocolListBox->addItem(tr("J1939"), QVariant(true));
  ui_->protocolListBox->setCurrentIndex(0);

  // Set default state for the checkboxes
  ui_->enhancedMetadataCheckbox->setChecked(use_enhanced_metadata_);

  // Load settings if available
  QSettings settings;
  settings.beginGroup("CanDatabase");
  if (settings.contains("use_enhanced_metadata")) {
    bool use_metadata = settings.value("use_enhanced_metadata").toBool();
    ui_->enhancedMetadataCheckbox->setChecked(use_metadata);
    use_enhanced_metadata_ = use_metadata;
  }
  settings.endGroup();

  // Populate list with existing files
  for (const QString& file : existing_files) {
    ui_->dbcFilesList->addItem(file);
  }

  // Enable OK button if we have files
  ui_->okButton->setEnabled(!existing_files.isEmpty());

  connect(ui_->okButton, &QPushButton::clicked, this, &DialogSelectCanDatabase::Ok);
  connect(ui_->cancelButton, &QPushButton::clicked, this, &DialogSelectCanDatabase::Cancel);
  connect(ui_->addDatabaseButton, &QPushButton::clicked, this, &DialogSelectCanDatabase::AddDatabaseFile);
  connect(ui_->removeDatabaseButton, &QPushButton::clicked, this, &DialogSelectCanDatabase::RemoveSelectedDatabaseFile);
  connect(ui_->dbcFilesList, &QListWidget::itemSelectionChanged, this, &DialogSelectCanDatabase::UpdateButtonStates);

  // Connect checkbox signals
  connect(ui_->enhancedMetadataCheckbox, &QCheckBox::toggled,
          [this](bool checked) { this->use_enhanced_metadata_ = checked; });
}

QStringList DialogSelectCanDatabase::GetDatabaseLocations() const {
  return database_locations_;
}

CanFrameProcessor::CanProtocol DialogSelectCanDatabase::GetCanProtocol() const {
  return protocol_;
}

bool DialogSelectCanDatabase::UseEnhancedMetadata() const {
  return use_enhanced_metadata_;
}

DialogSelectCanDatabase::~DialogSelectCanDatabase() {
  delete ui_;
}

// Private slots
void DialogSelectCanDatabase::Ok() {
  auto protocol_text = ui_->protocolListBox->currentText().toStdString();
  if (protocol_text == "RAW") {
    protocol_ = CanFrameProcessor::CanProtocol::RAW;
  }
  else if (protocol_text == "NMEA2K") {
    protocol_ = CanFrameProcessor::CanProtocol::NMEA2K;
  }
  else if (protocol_text == "J1939") {
    protocol_ = CanFrameProcessor::CanProtocol::J1939;
  }

  // Save settings
  QSettings settings;
  settings.beginGroup("CanDatabase");
  settings.setValue("use_enhanced_metadata", use_enhanced_metadata_);
  settings.endGroup();

  accept();
}

void DialogSelectCanDatabase::Cancel() {
  reject();
}

void DialogSelectCanDatabase::AddDatabaseFile() {
  QStringList fileLocations =
      QFileDialog::getOpenFileNames(this, tr("Select CAN database"), QString(), tr("CAN database (*.dbc)"));

  if (!fileLocations.isEmpty()) {
    for (const QString& location : fileLocations) {
      if (!database_locations_.contains(location)) {
        database_locations_.append(location);
        ui_->dbcFilesList->addItem(location);
      }
    }

    // Enable OK button as soon as we have at least one DBC file
    ui_->okButton->setEnabled(!database_locations_.isEmpty());
  }
}

void DialogSelectCanDatabase::RemoveSelectedDatabaseFile() {
  int currentRow = ui_->dbcFilesList->currentRow();
  if (currentRow >= 0) {
    QString location = ui_->dbcFilesList->item(currentRow)->text();
    database_locations_.removeAll(location);
    delete ui_->dbcFilesList->takeItem(currentRow);

    // Disable OK button if we have no DBC files
    ui_->okButton->setEnabled(!database_locations_.isEmpty());
  }
}

void DialogSelectCanDatabase::UpdateButtonStates() {
  ui_->removeDatabaseButton->setEnabled(ui_->dbcFilesList->currentRow() >= 0);
}
