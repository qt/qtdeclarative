// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <qtest.h>
#include <QSignalSpy>
#include <QTimer>
#include <QTcpSocket>
#include <QTcpServer>
#include <QDebug>
#include <QBuffer>

#include <private/qpacketprotocol_p.h>
#include <private/qpacket_p.h>

#include "debugutil_p.h"

class tst_QPacketProtocol : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTcpServer> m_server;
    std::unique_ptr<QTcpSocket> m_client;
    std::unique_ptr<QTcpSocket> m_serverConn;

private slots:
    void init();
    void cleanup();

    void send();
    void packetsAvailable();
    void packetsAvailable_data();
    void read();
};

void tst_QPacketProtocol::init()
{
    m_server = std::make_unique<QTcpServer>(this);
    m_serverConn = nullptr;
    QVERIFY(m_server->listen(QHostAddress("127.0.0.1")));

    m_client = std::make_unique<QTcpSocket>(this);

    QSignalSpy serverSpy(m_server.get(), SIGNAL(newConnection()));
    QSignalSpy clientSpy(m_client.get(), SIGNAL(connected()));

    m_client->connectToHost(m_server->serverAddress(), m_server->serverPort());

    QVERIFY(clientSpy.size() > 0 || clientSpy.wait());
    QVERIFY(serverSpy.size() > 0 || serverSpy.wait());

    m_serverConn.reset(m_server->nextPendingConnection());
}

void tst_QPacketProtocol::cleanup()
{
    m_client.reset();
    m_serverConn.reset();
    m_server.reset();
}

void tst_QPacketProtocol::send()
{
    QPacketProtocol in(m_client.get());
    QPacketProtocol out(m_serverConn.get());

    QByteArray ba;
    int num;

    QPacket packet(QDataStream::Qt_DefaultCompiledVersion);
    packet << "Hello world" << 123;
    out.send(packet.data());

    QVERIFY(QQmlDebugTest::waitForSignal(&in, SIGNAL(readyRead())));

    QPacket p(QDataStream::Qt_DefaultCompiledVersion, in.read());
    p >> ba >> num;
    QCOMPARE(ba, QByteArray("Hello world") + '\0');
    QCOMPARE(num, 123);
}

void tst_QPacketProtocol::packetsAvailable()
{
    QFETCH(int, packetCount);

    QPacketProtocol out(m_client.get());
    QPacketProtocol in(m_serverConn.get());

    QCOMPARE(out.packetsAvailable(), qint64(0));
    QCOMPARE(in.packetsAvailable(), qint64(0));

    for (int i=0; i<packetCount; i++) {
        QPacket packet(QDataStream::Qt_DefaultCompiledVersion);
        packet << "Hello";
        out.send(packet.data());
    }

    QVERIFY(QQmlDebugTest::waitForSignal(&in, SIGNAL(readyRead())));
    QCOMPARE(in.packetsAvailable(), qint64(packetCount));
}

void tst_QPacketProtocol::packetsAvailable_data()
{
    QTest::addColumn<int>("packetCount");

    QTest::newRow("1") << 1;
    QTest::newRow("2") << 2;
    QTest::newRow("10") << 10;
}

void tst_QPacketProtocol::read()
{
    QPacketProtocol in(m_client.get());
    QPacketProtocol out(m_serverConn.get());

    QVERIFY(in.read().isEmpty());

    QPacket packet(QDataStream::Qt_DefaultCompiledVersion);
    packet << 123;
    out.send(packet.data());

    QPacket packet2(QDataStream::Qt_DefaultCompiledVersion);
    packet2 << 456;
    out.send(packet2.data());
    QVERIFY(QQmlDebugTest::waitForSignal(&in, SIGNAL(readyRead())));

    int num;

    QPacket p1(QDataStream::Qt_DefaultCompiledVersion, in.read());
    QVERIFY(!p1.atEnd());
    p1 >> num;
    QCOMPARE(num, 123);

    QPacket p2(QDataStream::Qt_DefaultCompiledVersion, in.read());
    QVERIFY(!p2.atEnd());
    p2 >> num;
    QCOMPARE(num, 456);

    QVERIFY(in.read().isEmpty());
}

QTEST_MAIN(tst_QPacketProtocol)

#include "tst_qpacketprotocol.moc"
