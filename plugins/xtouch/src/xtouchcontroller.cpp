/*
  Q Light Controller
  xtouchcontroller.cpp

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

#include "xtouchcontroller.h"
#include "xtouchconstants.h"

#include <QMutexLocker>
#include <QStringList>
#include <QDebug>
#include <stddef.h>

#define DEFAULT_IP_ADDR 0xC0A8027F


/****************************************************************************
 * Initialization
 ****************************************************************************/

XtouchController::XtouchController(quint32 input, quint32 universe, QObject* parent)
    : QObject(parent)
    , m_connectedCounter(0)
    , m_handshakeTimer(NULL)
    , m_udpSocket(NULL)
    , m_universe(universe)
    , m_input(input)
    , m_ipAddr(DEFAULT_IP_ADDR)
{

    m_xtouchData.bank = 0u;
    qDebug() << "[Xtouch Controller] IP Address:" << input << " Broadcast address:" << universe << "(MAC:)";
}

XtouchController::~XtouchController()
{

}

/****************************************************************************
 * Open & Close
 ****************************************************************************/

bool XtouchController::open()
{
    if(nullptr == m_handshakeTimer)
    {
        m_handshakeTimer = new QTimer(this);
        m_handshakeTimer->setInterval(2000);
        connect(m_handshakeTimer, SIGNAL(timeout()), this, SLOT(handshakeTimer()));
        m_handshakeTimer->start();
    }

    QSharedPointer<QUdpSocket> udpSocket(m_udpSocket);
    if (nullptr == udpSocket)
    {
        udpSocket = QSharedPointer<QUdpSocket>(new QUdpSocket());
        m_udpSocket = udpSocket.toWeakRef();
        m_udpSocket->bind(10111, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
        connect(m_udpSocket.data(), SIGNAL(readyRead()), this, SLOT(dataReceived()));
    }

    memset(&m_xtouchData, 0, sizeof(m_xtouchData));
    memset(&m_xtouchOutData, 0, sizeof(m_xtouchOutData));

    writeConsole();

    return true;
}

bool XtouchController::close()
{
    if(nullptr != m_handshakeTimer)
    {
        m_handshakeTimer->stop();
        free(m_handshakeTimer);
        m_handshakeTimer = nullptr;
    }
    return true;
}

bool XtouchController::isConnected()
{
    if(m_connectedCounter > 0u)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool XtouchController::writeOutput(const QByteArray &data)
{
    Q_UNUSED(data);
    return true;
}

bool XtouchController::writeFeedback(quint32 channel, uchar value)
{
    uint8_t *data = (uint8_t*)&m_xtouchOutData;
    if(channel < sizeof(m_xtouchData))
    {
        data[channel] = value;

        m_xtouchData.wheel = m_xtouchOutData.wheel;

        for(uint8_t i = 0; i < BANK_COUNT; i ++)
        {
            for(uint8_t j = 0; j < CHANNEL_COUNT; j ++)
            {
                m_xtouchData.banks[i].channelStrip[j].knob = m_xtouchOutData.banks[i].channelStrip[j].knob;
                m_xtouchData.banks[i].channelStrip[j].fader = m_xtouchOutData.banks[i].channelStrip[j].fader;
            }
        }
    }

    writeConsole();

    return true;
}

void XtouchController::setIpAddr(uint32_t ipAddr)
{
    m_ipAddr = ipAddr;
}

uint32_t XtouchController::getIpAddr()
{
    return m_ipAddr;
}

void XtouchController::handshakeTimer()
{
    QByteArray packet;
    packet.clear();
    packet.append(xTouchHandshakePcToXtouch, sizeof(xTouchHandshakePcToXtouch));
    m_udpSocket->writeDatagram(packet, QHostAddress(m_ipAddr), 10111u);

    if(m_connectedCounter > 0u)
    {
        m_connectedCounter --;
    }
}

void XtouchController::dataReceived()
{
    QUdpSocket* udpSocket = qobject_cast<QUdpSocket*>(sender());
    Q_ASSERT(udpSocket != NULL);
    QByteArray packet;

    QByteArray datagram;
    QHostAddress senderAddress;
    while (udpSocket->hasPendingDatagrams())
    {
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddress);

        while(datagram.size() >= 3)
        {
            if(datagram[0] == char(0xb0))
            {
                readEncoder(datagram, m_xtouchData.bank);
                datagram.remove(0, 3);
            }
            else if(datagram[0] == char(0x90))
            {
                readButton(datagram, m_xtouchData.bank);
                datagram.remove(0, 3);
            }
            else if((uint8_t(datagram[0]) >= 0xe0) && (uint8_t(datagram[0]) <= 0xe8))
            {
                readFader(datagram, m_xtouchData.bank);
                datagram.remove(0, 3);
            }
            else if(datagram.startsWith(xTouchHandshakeXtouchToPc))
            {
                m_connectedCounter = 4;
                break;
            }
            else
            {
                break;
            }
        }
    }
}

void XtouchController::writeConsole()
{
    QByteArray packet;
    packet.clear();

    setBank(&packet, m_xtouchData.bank);
    setBankSelectors(&packet, m_xtouchData.bank);
    setButtons(&packet, m_xtouchData.bank);

    m_udpSocket->writeDatagram(packet, QHostAddress(m_ipAddr), 10111u);

    packet.clear();
    for(uint32_t i = 0u; i < sizeof(m_xtouchData.buttons); i ++)
    {
        if(xTouchButtonMap[i] != 0u)
        {
            packet.append(0x90);
            packet.append(xTouchButtonMap[i]);
            packet.append(m_xtouchOutData.buttons[i]);
        }
    }
    m_udpSocket->writeDatagram(packet, QHostAddress(m_ipAddr), 10111u);
}

void XtouchController::setBank(QByteArray *packet, uint8_t bank)
{
    for(uint8_t i = 0; i < 8; i ++)
    {
        setFader(packet, i, (uint16_t)m_xtouchData.banks[bank].channelStrip[i].fader << 8u);
        setKnob(packet, i, (uint16_t)m_xtouchData.banks[bank].channelStrip[i].knob);
    }
    setAssignmentDisp(packet, bank);
}

void XtouchController::setFader(QByteArray *packet, uint8_t channel, uint16_t value)
{
    if(channel <= 8u)
    {
        packet->append(0xe0 + channel);
        packet->append(0x7F & (value>>2));  /* lower nibble     */
        packet->append(value>>9);           /* higher nibble    */
    }
}

void XtouchController::setKnob(QByteArray *packet, uint8_t channel, uint8_t value)
{
    uint8_t numLeds = value / 22;
    if(value == 255)
    {
        numLeds = 13;
    }

    if(channel <= 8u)
    {
        packet->append(0xb0);
        packet->append(0x30 + channel);
        if(numLeds > 7)
        {
            packet->append(xTouchIntToLedRing[7]);
            numLeds -= 7;
        }
        else
        {
            packet->append(xTouchIntToLedRing[numLeds]);
            numLeds = 0u;
        }

        packet->append(0xb0);
        packet->append(0x38 + channel);
        packet->append(xTouchIntToLedRing[numLeds]);
    }
}

void XtouchController::setButtons(QByteArray *packet, uint8_t bank)
{

    for(uint32_t i = 0u; i < CHANNEL_COUNT; i ++)
    {
        packet->append(0x90);
        packet->append(0x00 + i);
        packet->append(m_xtouchOutData.banks[bank].channelStrip[i].rec);
        packet->append(0x90);
        packet->append(0x08 + i);
        packet->append(m_xtouchOutData.banks[bank].channelStrip[i].solo);
        packet->append(0x90);
        packet->append(0x10 + i);
        packet->append(m_xtouchOutData.banks[bank].channelStrip[i].mute);
        packet->append(0x90);
        packet->append(0x18 + i);
        packet->append(m_xtouchOutData.banks[bank].channelStrip[i].select);
    }
}

void XtouchController::setAssignmentDisp(QByteArray *packet, uint8_t number)
{
    if(number <= 99)
    {
        packet->append(0xb0);
        packet->append(0x61);
        packet->append(xTouchIntTo7Seg[number%10]);
        packet->append(0xb0);
        packet->append(0x60);
        packet->append(xTouchIntTo7Seg[number/10]);
    }
}

void XtouchController::setBankSelectors(QByteArray *packet, uint8_t bank)
{
    for(uint8_t i = 0u; i < BANK_COUNT; i ++)
    {
        packet->append(0x90);
        packet->append(0x36 + i);
        if(bank == i)
        {
            packet->append(0x7f);
        }
        else
        {
            packet->append(char(0x00));
        }
    }
}

void XtouchController::readFader(QByteArray &packet, uint8_t bank)
{
    uint8_t value = (quint8(packet[2]) << 1u) + (0x01 & (quint8(packet[1]) >> 6u));
    uint32_t channel = 0u;
    bool freshValue = false;

    if(quint8(packet[0]) >= 0xe0 && quint8(packet[0]) < 0xe8)
    {
        uint8_t tempChannel = quint8(packet[0]) - 0xe0u;

        m_xtouchData.banks[bank].channelStrip[tempChannel].fader = value;
        m_xtouchOutData.banks[bank].channelStrip[tempChannel].fader = value;
        channel = offsetof(xtouchData_t, banks)
                        + (sizeof(xtouchBank_t) * bank)
                        + (sizeof(xtouchChannelStrip_t) * tempChannel)
                        + offsetof(xtouchChannelStrip_t, fader);
        freshValue = true;
    }
    else if(quint8(packet[0]) == 0xe8)
    {
        m_xtouchData.masterFader = value;
        m_xtouchOutData.masterFader = value;
        channel = offsetof(xtouchData_t, masterFader);
        freshValue = true;
    }

    if(true == freshValue)
    {
        emit valueChanged(m_universe, m_input, channel, (uchar)value);
    }
}

void XtouchController::readButton(QByteArray &packet, uint8_t bank)
{
    uint8_t value = (quint8(packet[2]) << 1u) + (0x01 & (quint8(packet[1]) >> 6u));
    uint32_t channel = 0u;
    bool freshValue = false;

    value = packet[2];

    if( uint8_t(packet[1]) == 0x2F
     && uint8_t(packet[2]) == 0x7F)
    {
        if(m_xtouchData.bank < (BANK_COUNT - 1u))
        {
            m_xtouchData.bank ++;
            writeConsole();
        }
    }
    else if(uint8_t(packet[1]) == 0x2E
         && uint8_t(packet[2]) == 0x7F)
    {
        if(m_xtouchData.bank > 0 )
        {
            m_xtouchData.bank --;
            writeConsole();
        }
    }
    else if (uint8_t(packet[1]) < 0x20)
    {
        uint8_t tempChannel = uint8_t(packet[1]) % 8u;
        channel = offsetof(xtouchData_t, banks)
                    + (sizeof(xtouchBank_t) * bank)
                    + (sizeof(xtouchChannelStrip_t) * tempChannel);
        freshValue = true;
        switch (uint8_t(packet[1]) / 8)
        {
        case 0:
            m_xtouchData.banks[bank].channelStrip[tempChannel].rec = value;
            channel += offsetof(xtouchChannelStrip_t, rec);
            break;
        case 1:
            m_xtouchData.banks[bank].channelStrip[tempChannel].solo = value;
            channel += offsetof(xtouchChannelStrip_t, solo);
            break;
        case 2:
            m_xtouchData.banks[bank].channelStrip[tempChannel].mute = value;
            channel += offsetof(xtouchChannelStrip_t, mute);
            break;
        case 3:
            m_xtouchData.banks[bank].channelStrip[tempChannel].select = value;
            channel += offsetof(xtouchChannelStrip_t, select);
            break;
        default:
            break;
        }
    }
    else if (uint8_t(packet[1]) >= 0x36u && uint8_t(packet[1]) <= 0x3d && value == 0x7F)
    {
        m_xtouchData.bank = uint8_t(packet[1]) - 0x36u;
        writeConsole();
    }
    else
    {
        if(xTouchButtonMapRev[uint8_t(packet[1])] != 0u)
        {
            m_xtouchData.buttons[xTouchButtonMapRev[uint8_t(packet[1])]] = value;
            channel = offsetof(xtouchData_t, buttons)
                            + xTouchButtonMapRev[uint8_t(packet[1])];
            freshValue = true;
        }
    }

    if(true == freshValue)
    {
        emit valueChanged(m_universe, m_input, channel, (uchar)value);
    }
}

void XtouchController::readEncoder(QByteArray &packet, uint8_t bank)
{
    uint8_t value = 0u;
    uint32_t channel = 0u;
    bool freshValue = false;

    if (uint8_t(packet[1]) >= 0x10u && uint8_t(packet[1]) < 0x18)
    {
        uint8_t tempChannel = uint8_t(packet[1]) - 0x10;

        if(uint8_t(packet[2]) < 0x40)
        {
            uint8_t increment = uint8_t(packet[2]);
            if((255 - m_xtouchData.banks[bank].channelStrip[tempChannel].knob) < increment)
            {
                m_xtouchData.banks[bank].channelStrip[tempChannel].knob = 255u;
            }
            else
            {
                m_xtouchData.banks[bank].channelStrip[tempChannel].knob += increment;
            }
        }
        else
        {
            uint8_t decrement = uint8_t(packet[2]) - 0x40;
            if(m_xtouchData.banks[bank].channelStrip[tempChannel].knob < decrement)
            {
                m_xtouchData.banks[bank].channelStrip[tempChannel].knob = 0u;
            }
            else
            {
                m_xtouchData.banks[bank].channelStrip[tempChannel].knob -= decrement;
            }
        }
        m_xtouchOutData.banks[bank].channelStrip[tempChannel].knob = m_xtouchData.banks[bank].channelStrip[tempChannel].knob;
        channel = offsetof(xtouchData_t, banks)
                + (sizeof(xtouchBank_t) * bank)
                + (sizeof(xtouchChannelStrip_t) * tempChannel)
                + offsetof(xtouchChannelStrip_t, knob);
        value = m_xtouchData.banks[bank].channelStrip[tempChannel].knob;
        writeConsole();
        freshValue = true;
    }
    else if(uint8_t(packet[1]) == 0x3c)
    {
    	if(uint8_t(packet[2]) < 0x40)
		{
			uint8_t increment = uint8_t(packet[2]);
			if((255 - m_xtouchData.wheel) < increment)
			{
				m_xtouchData.wheel = 255u;
			}
			else
			{
				m_xtouchData.wheel += increment;
			}
		}
    	else
    	{
    		uint8_t decrement = uint8_t(packet[2]) - 0x40;
			if(m_xtouchData.wheel < decrement)
			{
				m_xtouchData.wheel = 0u;
			}
			else
			{
				m_xtouchData.wheel -= decrement;
			}
    	}
        channel = offsetof(xtouchData_t, wheel);
        value = m_xtouchData.wheel;
        freshValue = true;
    }

    if(true == freshValue)
    {
        emit valueChanged(m_universe, m_input, channel, (uchar)value);
    }
}
