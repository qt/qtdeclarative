// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TESTHTTPSERVER_P_H
#define TESTHTTPSERVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QTcpServer>
#include <QUrl>
#include <QPair>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <private/qglobal_p.h>
#include <QObject>
#include <QSet>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE

class TestHTTPServer : public QObject
{
    Q_OBJECT
public:
    TestHTTPServer();

    bool listen();
    quint16 port() const;
    QUrl baseUrl() const;
    QUrl url(const QString &documentPath) const;
    QString urlString(const QString &documentPath) const;
    QString errorString() const;

    enum Mode { Normal, Delay, Disconnect };
    bool serveDirectory(const QString &, Mode = Normal);

    bool wait(const QUrl &expect, const QUrl &reply, const QUrl &body);
    bool hasFailed() const;

    void addAlias(const QString &filename, const QString &aliasName);
    void addRedirect(const QString &filename, const QString &redirectName);

    void registerFileNameForContentSubstitution(const QString &fileName);

    // In Delay mode, each item needs one call to this function to be sent
    void sendDelayedItem();

private Q_SLOTS:
    void newConnection();
    void disconnected();
    void readyRead();
    void sendOne();

private:
    enum State {
        AwaitingHeader,
        AwaitingData,
        Failed
    };

    void serveGET(QTcpSocket *, const QByteArray &);
    bool reply(QTcpSocket *, const QByteArray &);

    QList<QPair<QString, Mode> > m_directories;
    QHash<QTcpSocket *, QByteArray> m_dataCache;
    QList<QPair<QTcpSocket *, QByteArray> > m_toSend;
    QSet<QString> m_contentSubstitutedFileNames;

    struct WaitData {
        QList<QByteArray> headerExactMatches;
        QList<QByteArray> headerPrefixes;
        QByteArray body;
    } m_waitData;
    QByteArray m_replyData;
    QByteArray m_bodyData;
    QByteArray m_data;
    State m_state;

    QHash<QString, QString> m_aliases;
    QHash<QString, QString> m_redirects;

    QTcpServer m_server;
};

class ThreadedTestHTTPServer : public QThread
{
    Q_OBJECT
public:
    ThreadedTestHTTPServer(const QString &dir, TestHTTPServer::Mode mode = TestHTTPServer::Normal);
    ThreadedTestHTTPServer(const QHash<QString, TestHTTPServer::Mode> &dirs);
    ~ThreadedTestHTTPServer();

    QUrl baseUrl() const;
    QUrl url(const QString &documentPath) const;
    QString urlString(const QString &documentPath) const;

protected:
    void run() override;

private:
    void start();

    QHash<QString, TestHTTPServer::Mode> m_dirs;
    quint16 m_port;
    QMutex m_mutex;
    QWaitCondition m_condition;
};

QT_END_NAMESPACE

#endif // TESTHTTPSERVER_P_H

