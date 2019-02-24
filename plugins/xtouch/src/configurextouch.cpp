/*
  Q Light Controller Plus
  configureartnet.cpp

  Copyright (c) Massimo Callegari

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QSpacerItem>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QDebug>
#include <QHostAddress>

#include "configurextouch.h"
#include "xtouchplugin.h"

#define KNodesColumnIP          0
#define KNodesColumnShortName   1
#define KNodesColumnLongName    2

#define KMapColumnInterface     0
#define KMapColumnUniverse      1
#define KMapColumnIPAddress     2
#define KMapColumnArtNetUni     3
#define KMapColumnTransmitMode  4

#define PROP_UNIVERSE (Qt::UserRole + 0)
#define PROP_LINE (Qt::UserRole + 1)
#define PROP_TYPE (Qt::UserRole + 2)

// ArtNet universe is a 15bit value
#define ARTNET_UNIVERSE_MAX 0x7fff

/*****************************************************************************
 * Initialization
 *****************************************************************************/

ConfigureXtouch::ConfigureXtouch(XtouchPlugin* plugin, QWidget* parent)
        : QDialog(parent)
{
    Q_ASSERT(plugin != NULL);
    m_plugin = plugin;

    /* Setup UI controls */
    setupUi(this);
}

void ConfigureXtouch::showIPAlert(QString ip)
{
    QMessageBox::critical(this, tr("Invalid IP"), tr("%1 is not a valid IP.\nPlease fix it before confirming.").arg(ip));
}

ConfigureXtouch::~ConfigureXtouch()
{
}

/*****************************************************************************
 * Dialog actions
 *****************************************************************************/

void ConfigureXtouch::accept()
{
    uint32_t ipAddr = QHostAddress(m_tbxIpAddr->text()).toIPv4Address();

    m_plugin->setIpAddr(ipAddr);
    QDialog::accept();
}

int ConfigureXtouch::exec()
{
    return QDialog::exec();
}
