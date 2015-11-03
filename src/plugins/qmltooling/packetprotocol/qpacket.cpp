/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpacket_p.h"

QT_BEGIN_NAMESPACE

/*!
  \class QPacket
  \internal

  \brief The QPacket class encapsulates an unfragmentable packet of data to be
  transmitted by QPacketProtocol.

  The QPacket class works together with QPacketProtocol to make it simple to
  send arbitrary sized data "packets" across fragmented transports such as TCP
  and UDP.

  QPacket provides a QDataStream interface to an unfragmentable packet.
  Applications should construct a QPacket, propagate it with data and then
  transmit it over a QPacketProtocol instance.  For example:
  \code
  int version = QDataStream::Qt_DefaultCompiledVersion;
  QPacketProtocol protocol(...);

  QPacket myPacket(version);
  myPacket << "Hello world!" << 123;
  protocol.send(myPacket.data());
  \endcode

  As long as both ends of the connection are using the QPacketProtocol class
  and the same data stream version, the data within this packet will be
  delivered unfragmented at the other end, ready for extraction.

  \code
  QByteArray greeting;
  int count;

  QPacket myPacket(version, protocol.read());

  myPacket >> greeting >> count;
  \endcode

  Only packets constructed from raw byte arrays may be read from. Empty QPacket
  instances are for transmission only and are considered "write only". Attempting
  to read data from them will result in undefined behavior.

  \ingroup io
  \sa QPacketProtocol
 */


/*!
  Constructs an empty write-only packet.
  */
QPacket::QPacket(int version)
{
    buf.open(QIODevice::WriteOnly);
    setDevice(&buf);
    setVersion(version);
}

/*!
  Constructs a read-only packet.
 */
QPacket::QPacket(int version, const QByteArray &data)
{
    buf.setData(data);
    buf.open(QIODevice::ReadOnly);
    setDevice(&buf);
    setVersion(version);
}

/*!
  Returns raw packet data.
  */
QByteArray QPacket::data() const
{
    return buf.data();
}

QT_END_NAMESPACE
