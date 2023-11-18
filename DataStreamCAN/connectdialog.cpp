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

#include "connectdialog.h"
#include "ui_connectdialog.h"
#include "../PluginsCommonCAN/select_can_database.h"

#include <QCanBus>

ConnectDialog::ConnectDialog(QWidget *parent) : QDialog(parent),
                                                m_ui(new Ui::ConnectDialog)
{
    m_ui->setupUi(this);

    m_ui->errorFilterEdit->setValidator(new QIntValidator(0, 0x1FFFFFFFU, this));

    m_ui->loopbackBox->addItem(tr("unspecified"), QVariant());
    m_ui->loopbackBox->addItem(tr("false"), QVariant(false));
    m_ui->loopbackBox->addItem(tr("true"), QVariant(true));
    m_ui->loopbackBox->setCurrentIndex(1);

    m_ui->receiveOwnBox->addItem(tr("unspecified"), QVariant());
    m_ui->receiveOwnBox->addItem(tr("false"), QVariant(false));
    m_ui->receiveOwnBox->addItem(tr("true"), QVariant(true));
    m_ui->receiveOwnBox->setCurrentIndex(1);

    m_ui->bitrateBox->addItem(tr("125000"), QVariant(true));
    m_ui->bitrateBox->addItem(tr("250000"), QVariant(true));
    m_ui->bitrateBox->addItem(tr("500000"), QVariant(true));
    m_ui->bitrateBox->addItem(tr("1000000"), QVariant(true));
    m_ui->bitrateBox->setCurrentIndex(2);

    m_ui->okButton->setEnabled(false);

    connect(m_ui->okButton, &QPushButton::clicked, this, &ConnectDialog::ok);
    connect(m_ui->cancelButton, &QPushButton::clicked, this, &ConnectDialog::cancel);
    connect(m_ui->backendListBox, &QComboBox::currentTextChanged,
            this, &ConnectDialog::backendChanged);
    connect(m_ui->interfaceListBox, &QComboBox::currentTextChanged,
            this, &ConnectDialog::interfaceChanged);
    connect(m_ui->loadDatabaseButton, &QPushButton::clicked,
            this, &ConnectDialog::importDatabaseLocation);
    m_ui->rawFilterEdit->hide();
    m_ui->rawFilterLabel->hide();

    m_ui->backendListBox->addItems(QCanBus::instance()->plugins());

    updateSettings();
}

ConnectDialog::~ConnectDialog()
{
    delete m_ui;
}

ConnectDialog::Settings ConnectDialog::settings() const
{
    return m_currentSettings;
}

void ConnectDialog::backendChanged(const QString &backend)
{
    m_ui->interfaceListBox->clear();
    m_interfaces = QCanBus::instance()->availableDevices(backend);
    for (const QCanBusDeviceInfo &info : qAsConst(m_interfaces))
        m_ui->interfaceListBox->addItem(info.name());
}

void ConnectDialog::interfaceChanged(const QString &interface)
{

    for (const QCanBusDeviceInfo &info : qAsConst(m_interfaces))
    {
        if (info.name() == interface)
        {
            break;
        }
    }
}

void ConnectDialog::ok()
{
    updateSettings();
    accept();
}

void ConnectDialog::cancel()
{
    revertSettings();
    reject();
}

QString ConnectDialog::configurationValue(QCanBusDevice::ConfigurationKey key)
{
    QVariant result;

    for (const ConfigurationItem &item : qAsConst(m_currentSettings.configurations))
    {
        if (item.first == key)
        {
            result = item.second;
            break;
        }
    }

    if (result.isNull() && (key == QCanBusDevice::LoopbackKey ||
                            key == QCanBusDevice::ReceiveOwnKey))
    {
        return tr("unspecified");
    }

    return result.toString();
}

void ConnectDialog::revertSettings()
{
    m_ui->backendListBox->setCurrentText(m_currentSettings.backendName);
    m_ui->interfaceListBox->setCurrentText(m_currentSettings.deviceInterfaceName);

    QString value = configurationValue(QCanBusDevice::LoopbackKey);
    m_ui->loopbackBox->setCurrentText(value);

    value = configurationValue(QCanBusDevice::ReceiveOwnKey);
    m_ui->receiveOwnBox->setCurrentText(value);

    value = configurationValue(QCanBusDevice::ErrorFilterKey);
    m_ui->errorFilterEdit->setText(value);

    value = configurationValue(QCanBusDevice::BitRateKey);
    m_ui->bitrateBox->setCurrentText(value);
}

void ConnectDialog::updateSettings()
{
    m_currentSettings.backendName = m_ui->backendListBox->currentText();
    m_currentSettings.deviceInterfaceName = m_ui->interfaceListBox->currentText();

    if (m_currentSettings.useConfigurationEnabled)
    {
        m_currentSettings.configurations.clear();
        // process LoopBack
        if (m_ui->loopbackBox->currentIndex() != 0)
        {
            ConfigurationItem item;
            item.first = QCanBusDevice::LoopbackKey;
            item.second = m_ui->loopbackBox->currentData();
            m_currentSettings.configurations.append(item);
        }

        // process ReceiveOwnKey
        if (m_ui->receiveOwnBox->currentIndex() != 0)
        {
            ConfigurationItem item;
            item.first = QCanBusDevice::ReceiveOwnKey;
            item.second = m_ui->receiveOwnBox->currentData();
            m_currentSettings.configurations.append(item);
        }

        // process error filter
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

        // process raw filter list
        if (!m_ui->rawFilterEdit->text().isEmpty())
        {
            //TODO current ui not sfficient to reflect this param
        }

        // process bitrate
        const int bitrate = m_ui->bitrateBox->currentText().toInt();
        if (bitrate > 0)
        {
            const ConfigurationItem item(QCanBusDevice::BitRateKey, QVariant(bitrate));
            m_currentSettings.configurations.append(item);
        }
    }
}

void ConnectDialog::importDatabaseLocation()
{
    DialogSelectCanDatabase* dialog = new DialogSelectCanDatabase();
    if (dialog->exec() != static_cast<int>(QDialog::Accepted))
    {
        ConnectDialog::cancel();
        return;
    }

    m_currentSettings.canDatabaseLocation = dialog->GetDatabaseLocation();
    m_currentSettings.protocol = dialog->GetCanProtocol();
    // Since file is gotten, enable ok button.
    m_ui->okButton->setEnabled(true);
}
