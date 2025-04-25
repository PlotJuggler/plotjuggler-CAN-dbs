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

#include <QCanBus>
#include <QDebug>
#include <QSettings>

#include "connectdialog.h"
#include "ui_connectdialog.h"
#include "../PluginsCommonCAN/select_can_database.h"

ConnectDialog::ConnectDialog(QWidget *parent) : QDialog(parent),
                                                m_ui(new Ui::ConnectDialog)
{
    m_ui->setupUi(this);

    m_ui->errorFilterEdit->setValidator(new QIntValidator(0, 0x1FFFFFFFU, this));

    // Loopback options
    m_ui->loopbackBox->addItem(tr("unspecified"), QVariant());
    m_ui->loopbackBox->addItem(tr("false"), QVariant(false));
    m_ui->loopbackBox->addItem(tr("true"), QVariant(true));
    m_ui->loopbackBox->setCurrentIndex(1);

    // Receive Own options
    m_ui->receiveOwnBox->addItem(tr("unspecified"), QVariant());
    m_ui->receiveOwnBox->addItem(tr("false"), QVariant(false));
    m_ui->receiveOwnBox->addItem(tr("true"), QVariant(true));
    m_ui->receiveOwnBox->setCurrentIndex(1);

    // Bitrate options
    m_ui->bitrateBox->addItem("125000", QVariant(125000));
    m_ui->bitrateBox->addItem("250000", QVariant(250000));
    m_ui->bitrateBox->addItem("500000", QVariant(500000));
    m_ui->bitrateBox->addItem("1000000", QVariant(1000000));
    m_ui->bitrateBox->setCurrentIndex(2); // 500000 by default

    // Connect signals
    connect(m_ui->okButton, &QPushButton::clicked, this, &ConnectDialog::ok);
    connect(m_ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_ui->backendListBox, &QComboBox::currentTextChanged,
            this, &ConnectDialog::backendChanged);
    connect(m_ui->interfaceListBox, &QComboBox::currentTextChanged,
            this, &ConnectDialog::interfaceChanged);
    connect(m_ui->loadDatabaseButton, &QPushButton::clicked,
            this, &ConnectDialog::importDatabaseLocation);

    // Hide raw filter options
    m_ui->rawFilterEdit->hide();
    m_ui->rawFilterLabel->hide();

    // Populate backend list
    QStringList backends = QCanBus::instance()->plugins();
    if (!backends.isEmpty())
    {
        m_ui->backendListBox->addItems(backends);
    }
    else
    {
        qWarning() << "No CAN backends found";
    }

    // Load settings from QSettings
    loadSettings();

    // Enable OK button if we have DBC files
    m_ui->okButton->setEnabled(!m_currentSettings.canDatabaseLocations.isEmpty());
}

ConnectDialog::~ConnectDialog()
{
    delete m_ui;
}

void ConnectDialog::loadSettings()
{
    QSettings settings;
    settings.beginGroup("DataStreamCAN");

    // Load backend
    if (settings.contains("backend"))
    {
        QString backend = settings.value("backend").toString();
        int index = m_ui->backendListBox->findText(backend);
        if (index >= 0)
        {
            m_ui->backendListBox->setCurrentIndex(index);
        }
    }

    // Load interface
    if (settings.contains("interface"))
    {
        m_ui->interfaceListBox->setEditText(settings.value("interface").toString());
    }

    // Load loopback setting
    if (settings.contains("loopback"))
    {
        bool loopback = settings.value("loopback").toBool();
        m_ui->loopbackBox->setCurrentIndex(loopback ? 2 : 1);
    }

    // Load receive own setting
    if (settings.contains("receive_own"))
    {
        bool receiveOwn = settings.value("receive_own").toBool();
        m_ui->receiveOwnBox->setCurrentIndex(receiveOwn ? 2 : 1);
    }

    // Load error filter
    if (settings.contains("error_filter"))
    {
        m_ui->errorFilterEdit->setText(settings.value("error_filter").toString());
    }

    // Load bitrate
    if (settings.contains("bitrate"))
    {
        QString bitrate = settings.value("bitrate").toString();
        int bitrateIndex = m_ui->bitrateBox->findText(bitrate);
        if (bitrateIndex >= 0)
        {
            m_ui->bitrateBox->setCurrentIndex(bitrateIndex);
        }
    }

    // Load CAN database files
    int dbcCount = settings.beginReadArray("dbc_files");
    QStringList dbcFiles;
    for (int i = 0; i < dbcCount; ++i)
    {
        settings.setArrayIndex(i);
        dbcFiles.append(settings.value("path").toString());
    }
    settings.endArray();

    if (!dbcFiles.isEmpty())
    {
        m_currentSettings.canDatabaseLocations = dbcFiles;
    }

    // Load protocol
    if (settings.contains("protocol"))
    {
        QString protocol = settings.value("protocol").toString();
        if (protocol == "RAW")
            m_currentSettings.protocol = CanFrameProcessor::RAW;
        else if (protocol == "NMEA2K")
            m_currentSettings.protocol = CanFrameProcessor::NMEA2K;
        else if (protocol == "J1939")
            m_currentSettings.protocol = CanFrameProcessor::J1939;
    }

    settings.endGroup();

    // Update the rest of the settings to match UI
    updateSettings();
}

void ConnectDialog::saveSettings()
{
    QSettings settings;
    settings.beginGroup("DataStreamCAN");

    settings.setValue("backend", m_currentSettings.backendName);
    settings.setValue("interface", m_currentSettings.deviceInterfaceName);

    for (const ConfigurationItem &item : m_currentSettings.configurations)
    {
        if (item.first == QCanBusDevice::LoopbackKey)
        {
            settings.setValue("loopback", item.second.toBool());
        }
        else if (item.first == QCanBusDevice::ReceiveOwnKey)
        {
            settings.setValue("receive_own", item.second.toBool());
        }
        else if (item.first == QCanBusDevice::ErrorFilterKey)
        {
            settings.setValue("error_filter", item.second.toString());
        }
        else if (item.first == QCanBusDevice::BitRateKey)
        {
            settings.setValue("bitrate", item.second.toString());
        }
    }

    // Save protocol
    QString protocol;
    switch (m_currentSettings.protocol)
    {
    case CanFrameProcessor::RAW:
        protocol = "RAW";
        break;
    case CanFrameProcessor::NMEA2K:
        protocol = "NMEA2K";
        break;
    case CanFrameProcessor::J1939:
        protocol = "J1939";
        break;
    default:
        protocol = "RAW";
    }
    settings.setValue("protocol", protocol);

    // Save CAN database files
    settings.beginWriteArray("dbc_files");
    for (int i = 0; i < m_currentSettings.canDatabaseLocations.size(); ++i)
    {
        settings.setArrayIndex(i);
        settings.setValue("path", m_currentSettings.canDatabaseLocations.at(i));
    }
    settings.endArray();

    settings.endGroup();
}

ConnectDialog::Settings ConnectDialog::settings() const
{
    return m_currentSettings;
}

void ConnectDialog::applySettings(const Settings &settings)
{
    m_currentSettings = settings;

    // Set backend
    int backendIndex = m_ui->backendListBox->findText(settings.backendName);
    if (backendIndex >= 0)
    {
        m_ui->backendListBox->setCurrentIndex(backendIndex);
    }

    // Set interface
    backendChanged(settings.backendName);
    int interfaceIndex = m_ui->interfaceListBox->findText(settings.deviceInterfaceName);
    if (interfaceIndex >= 0)
    {
        m_ui->interfaceListBox->setCurrentIndex(interfaceIndex);
    }
    else if (!settings.deviceInterfaceName.isEmpty())
    {
        m_ui->interfaceListBox->setEditText(settings.deviceInterfaceName);
    }

    // Set configuration parameters
    for (const ConfigurationItem &item : settings.configurations)
    {
        if (item.first == QCanBusDevice::LoopbackKey)
        {
            bool loopback = item.second.toBool();
            m_ui->loopbackBox->setCurrentIndex(loopback ? 2 : 1);
        }
        else if (item.first == QCanBusDevice::ReceiveOwnKey)
        {
            bool receiveOwn = item.second.toBool();
            m_ui->receiveOwnBox->setCurrentIndex(receiveOwn ? 2 : 1);
        }
        else if (item.first == QCanBusDevice::ErrorFilterKey)
        {
            m_ui->errorFilterEdit->setText(item.second.toString());
        }
        else if (item.first == QCanBusDevice::BitRateKey)
        {
            QString bitrate = item.second.toString();
            int bitrateIndex = m_ui->bitrateBox->findText(bitrate);
            if (bitrateIndex >= 0)
            {
                m_ui->bitrateBox->setCurrentIndex(bitrateIndex);
            }
        }
    }

    // Enable OK button if we have database files
    m_ui->okButton->setEnabled(!settings.canDatabaseLocations.isEmpty());
}

void ConnectDialog::backendChanged(const QString &backend)
{
    m_ui->interfaceListBox->clear();
    m_interfaces = QCanBus::instance()->availableDevices(backend);
    for (const QCanBusDeviceInfo &info : qAsConst(m_interfaces))
        m_ui->interfaceListBox->addItem(info.name());
}

void ConnectDialog::ok()
{
    updateSettings();
    saveSettings();
    accept();
}

void ConnectDialog::updateSettings()
{
    m_currentSettings.backendName = m_ui->backendListBox->currentText();
    m_currentSettings.deviceInterfaceName = m_ui->interfaceListBox->currentText();
    m_currentSettings.useConfigurationEnabled = true;
    m_currentSettings.configurations.clear();

    // Process loopback setting
    if (m_ui->loopbackBox->currentIndex() != 0)
    {
        ConfigurationItem item;
        item.first = QCanBusDevice::LoopbackKey;
        item.second = m_ui->loopbackBox->currentData();
        m_currentSettings.configurations.append(item);
    }

    // Process receive own setting
    if (m_ui->receiveOwnBox->currentIndex() != 0)
    {
        ConfigurationItem item;
        item.first = QCanBusDevice::ReceiveOwnKey;
        item.second = m_ui->receiveOwnBox->currentData();
        m_currentSettings.configurations.append(item);
    }

    // Process error filter
    if (!m_ui->errorFilterEdit->text().isEmpty())
    {
        QString value = m_ui->errorFilterEdit->text();
        bool ok = false;
        int dec = value.toInt(&ok);
        if (ok)
        {
            ConfigurationItem item;
            item.first = QCanBusDevice::ErrorFilterKey;
            item.second = QVariant::fromValue(QCanBusFrame::FrameErrors(dec));
            m_currentSettings.configurations.append(item);
        }
    }

    // Process bitrate setting
    const int bitrate = m_ui->bitrateBox->currentText().toInt();
    if (bitrate > 0)
    {
        ConfigurationItem item;
        item.first = QCanBusDevice::BitRateKey;
        item.second = QVariant(bitrate);
        m_currentSettings.configurations.append(item);
    }

    // Add support for flexible data rate (CAN FD)
    ConfigurationItem fdItem;
    fdItem.first = QCanBusDevice::CanFdKey;
    fdItem.second = QVariant(true);
    m_currentSettings.configurations.append(fdItem);
}

void ConnectDialog::importDatabaseLocation()
{
    DialogSelectCanDatabase *dialog = new DialogSelectCanDatabase(m_currentSettings.canDatabaseLocations);
    if (dialog->exec() != static_cast<int>(QDialog::Accepted))
    {
        delete dialog;
        return;
    }

    m_currentSettings.canDatabaseLocations = dialog->GetDatabaseLocations();
    m_currentSettings.protocol = dialog->GetCanProtocol();

    // Enable OK button if we have database files
    m_ui->okButton->setEnabled(!m_currentSettings.canDatabaseLocations.isEmpty());

    delete dialog;
}
