/*
  Q Light Controller Plus
  xtouchplugin.cpp

  Copyright (c) Stefan Strobel (derived from Massimo Callegari)
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

#include "xtouchplugin.h"
#include "configurextouch.h"
#include "xtouchcontroller.h"

#include <QDebug>


XtouchPlugin::~XtouchPlugin()
{
    /* nothing to do here all ressources should be freed until here */
}

void XtouchPlugin::init()
{
    foreach(QNetworkInterface interface, QNetworkInterface::allInterfaces())
    {
        foreach (QNetworkAddressEntry entry, interface.addressEntries())
        {
            QHostAddress addr = entry.ip();
            if (addr.protocol() != QAbstractSocket::IPv6Protocol)
            {
                XtouchIO tmpIO;
                tmpIO.interface = interface;
                tmpIO.address = entry;
                tmpIO.controller = nullptr;

                bool alreadyInList = false;
                for(int j = 0; j < m_IOmapping.count(); j++)
                {
                    if (m_IOmapping.at(j).address == tmpIO.address)
                    {
                        alreadyInList = true;
                        break;
                    }
                }
                if (alreadyInList == false)
                {
                    m_IOmapping.append(tmpIO);
                }
            }
        }
    }
}

QString XtouchPlugin::name()
{
    return QString("Xtouch");
}

int XtouchPlugin::capabilities() const
{
    return QLCIOPlugin::Output | QLCIOPlugin::Input | QLCIOPlugin::Feedback;
}

QString XtouchPlugin::pluginInfo()
{
    QString str;

    str += QString("<HTML>");
    str += QString("<HEAD>");
    str += QString("<TITLE>%1</TITLE>").arg(name());
    str += QString("</HEAD>");
    str += QString("<BODY>");

    str += QString("<P>");
    str += QString("<H3>%1</H3>").arg(name());
    str += tr("This plugin provides supports control from a Behringer X-Touch via Network.");
    str += QString("</P>");

    return str;
}

/*********************************************************************
 * Outputs
 *********************************************************************/
QStringList XtouchPlugin::outputs()
{
    QStringList list;
    int j = 0;

    init();

    foreach (XtouchIO line, m_IOmapping)
    {
        list << QString("%1: %2").arg(j + 1).arg(line.address.ip().toString());
        j++;
    }
    return list;
}

QString XtouchPlugin::outputInfo(quint32 output)
{
    if ((output >= (quint32)m_IOmapping.length()) || (NULL == m_IOmapping[output].controller))
    {
        return QString();
    }

    QString str;

    str += QString("<H3>%1 %2</H3>").arg(tr("Output")).arg(outputs()[output]);
    XtouchController *ctrl = m_IOmapping.at(output).controller;
    if (ctrl == NULL)
        str += tr("Status: Not open");
    else
    {
        str += tr("Status: Open");
        str += QString("<BR>");

        QString boundString;
        if (ctrl->isConnected())
            str += QString("connected");
        else
        {
            str += QString("Not connected");
        }
        str += QString("<BR>");
    }

    str += QString("</P>");
    str += QString("</BODY>");
    str += QString("</HTML>");

    return str;
}

bool XtouchPlugin::openOutput(quint32 output, quint32 universe)
{
    if (output >= (quint32)m_IOmapping.count())
        return false;

    if(m_IOmapping[output].controller == NULL)
    {
        m_IOmapping[output].controller = new XtouchController(output, universe, this);
        connect(m_IOmapping[output].controller, SIGNAL(valueChanged(quint32,quint32,quint32,uchar)),
                        this, SIGNAL(valueChanged(quint32,quint32,quint32,uchar)));
    }

    m_IOmapping[output].controller->open();
    qDebug() << "[ArtNet] Open output on address :" << m_IOmapping.at(output).address.ip().toString();

    return true;
}

void XtouchPlugin::closeOutput(quint32 output, quint32 universe)
{
    (void)universe;
    if ((output >= (quint32)m_IOmapping.length()) || (NULL == m_IOmapping[output].controller))
        return;
    m_IOmapping[output].controller->close();
}

void XtouchPlugin::writeUniverse(quint32 universe, quint32 output, const QByteArray &data)
{
    (void)universe;
    (void)data;
    if (output >= (quint32)m_IOmapping.count())
        return;
    m_IOmapping[output].controller->writeOutput(data);
}

/*************************************************************************
  * Inputs
  *************************************************************************/
QStringList XtouchPlugin::inputs()
{
    QStringList list;
    int j = 0;

    init();

    foreach (XtouchIO line, m_IOmapping)
    {
        list << QString("%1: %2").arg(j + 1).arg(line.address.ip().toString());
        j++;
    }
    return list;
}

bool XtouchPlugin::openInput(quint32 input, quint32 universe)
{
    if (input >= (quint32)m_IOmapping.count())
        return false;

    if(m_IOmapping[input].controller == NULL)
    {
        m_IOmapping[input].controller = new XtouchController(input, universe, this);
        connect(m_IOmapping[input].controller, SIGNAL(valueChanged(quint32,quint32,quint32,uchar)),
                        this, SIGNAL(valueChanged(quint32,quint32,quint32,uchar)));
    }

    m_IOmapping[input].controller->open();

    return true;
}

void XtouchPlugin::closeInput(quint32 input, quint32 universe)
{
    (void)universe;
    if ((input >= (quint32)m_IOmapping.length()) || (NULL == m_IOmapping[input].controller))
        return;

    m_IOmapping[input].controller->close();
}

QString XtouchPlugin::inputInfo(quint32 input)
{
    if ((input >= (quint32)m_IOmapping.length()) || (NULL == m_IOmapping[input].controller))
    {
        return QString();
    }

    QString str;

    str += QString("<H3>%1 %2</H3>").arg(tr("Output")).arg(outputs()[input]);
    XtouchController *ctrl = m_IOmapping.at(input).controller;
    if (ctrl == NULL)
        str += tr("Status: Not open");
    else
    {
        str += tr("Status: Open");
        str += QString("<BR>");

        QString boundString;
        if (ctrl->isConnected())
            str += QString("connected");
        else
        {
            str += QString("Not connected");
        }
        str += QString("<BR>");
    }

    str += QString("</P>");
    str += QString("</BODY>");
    str += QString("</HTML>");

    return str;
}

void XtouchPlugin::sendFeedBack(quint32 universe, quint32 inputLine,
                               quint32 channel, uchar value, const QString &key)
{
    Q_UNUSED(universe)
    Q_UNUSED(key)
    m_IOmapping[inputLine].controller->writeFeedback(channel, value);
}


/*********************************************************************
 * Configuration
 *********************************************************************/
void XtouchPlugin::configure()
{
    ConfigureXtouch conf(this);
    conf.exec();
}

bool XtouchPlugin::canConfigure()
{
    return false;
}

void XtouchPlugin::setParameter(quint32 universe, quint32 line, Capability type,
                                QString name, QVariant value)
{
    (void)universe;
    (void)type;
    (void)name;
    (void)value;
    if (line >= (quint32)m_IOmapping.length())
        return;

}

QList<XtouchIO> XtouchPlugin::getIOMapping()
{
    return m_IOmapping;
}

void XtouchPlugin::setIpAddr(uint32_t ipAddr)
{
    /* just for the start - this should configure each controller seperately */
    foreach(XtouchIO io, m_IOmapping)
    {
        if(nullptr != io.controller)
        {
            io.controller->setIpAddr((ipAddr));
        }
    }
}


/*****************************************************************************
 * Plugin export
 ****************************************************************************/
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN2(artnetplugin, ArtNetPlugin)
#endif
