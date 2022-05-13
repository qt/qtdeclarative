// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
  Returns a reference to the raw packet data.
  */
const QByteArray &QPacket::data() const
{
    return buf.data();
}

/*!
  Returns a copy of the raw packet data, with extra reserved space removed.
  Mind that this triggers a deep copy. Use it if you anticipate the data to be detached soon anyway.
  */
QByteArray QPacket::squeezedData() const
{
    QByteArray ret = buf.data();
    ret.squeeze();
    return ret;
}

/*!
  Clears the packet, discarding any data.
 */
void QPacket::clear()
{
    buf.reset();
    QByteArray &buffer = buf.buffer();
    // Keep the old size to prevent unnecessary allocations
    buffer.reserve(buffer.capacity());
    buffer.truncate(0);
}

QT_END_NAMESPACE
