/*
  Q Light Controller
  xtouchcontroller.h

  Copyright (C) Stefan Strobel

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

#ifndef XTOUCHCONTROLLER_H
#define XTOUCHCONTROLLER_H

#if defined(ANDROID)
#include <QNetworkInterface>
#include <QHostAddress>
#include <QUdpSocket>
#include <QScopedPointer>
#include <QSharedPointer>
#else
#include <QtNetwork>
#endif
#include <QMutex>
#include <QTimer>

#define CHANNEL_COUNT   8u
#define BANK_COUNT      8u

typedef struct xtouchChannelStrip
{
    uint8_t fader;
    uint8_t knob;
    uint8_t rec;
    uint8_t solo;
    uint8_t mute;
    uint8_t select;
}xtouchChannelStrip_t;

typedef struct xtouchBank
{
    xtouchChannelStrip_t channelStrip[CHANNEL_COUNT];
}xtouchBank_t;

typedef struct xtouchData
{
    xtouchBank_t banks[BANK_COUNT];
    uint8_t masterFader;
    uint8_t wheel;
    uint8_t bank;
    uint8_t buttons[56];
}xtouchData_t;


class XtouchController : public QObject
{
    Q_OBJECT

    /************************************************************************
     * Initialization
     ************************************************************************/
public:
    /**
     * Construct a new XtouchController object
     *
     * @param parent The owner of this object
     */
    XtouchController(quint32 input, quint32 universe, QObject* parent = 0);

    /** Destructor */
    virtual ~XtouchController();

    /************************************************************************
     * Open & Close
     ************************************************************************/
public:
    /** @reimp */
    bool open();

    /** @reimp */
    bool close();

    /** */
    bool isConnected();

    /** */
    bool writeOutput(const QByteArray &data);

private:
    /** */
    void writeConsole();
    void setBank(QByteArray *packet, uint8_t bank);
    void setFader(QByteArray *packet, uint8_t channel, uint16_t value);
    void setKnob(QByteArray *packet, uint8_t channel, uint8_t value);
    void setButtons(QByteArray *packet, uint8_t bank);
    void setAssignmentDisp(QByteArray *packet, uint8_t number);
    void setBankSelectors(QByteArray *packet, uint8_t bank);
    void readFader(QByteArray &packet, uint8_t bank);
    void readButton(QByteArray &packet, uint8_t bank);
    void readEncoder(QByteArray &packet, uint8_t bank);

protected slots:
    void handshakeTimer();
    void dataReceived();

protected:
    quint16 m_connectedCounter;
    QTimer* m_handshakeTimer;
private:
    QSharedPointer<QUdpSocket> m_udpSocket;
    xtouchData_t m_xtouchData;
    xtouchData_t m_xtouchOutData;
    uint32_t m_universe;
    uint32_t m_input;

signals:
    void valueChanged(quint32 universe, quint32 input, quint32 channel, uchar value);
};

#endif
