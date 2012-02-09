/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PROFILECLIENT_H
#define PROFILECLIENT_H

#include "profiledata.h"

#include <QtDeclarative/private/qdeclarativedebugclient_p.h>
#include <QtDeclarative/private/qdeclarativeprofilerservice_p.h>

class ProfileClientPrivate;
class ProfileClient : public QDeclarativeDebugClient
{
    Q_OBJECT

    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool recording READ isRecording WRITE setRecording
               NOTIFY recordingChanged)

public:
    ProfileClient(const QString & clientName,
                  QDeclarativeDebugConnection *client);
    ~ProfileClient();

    bool isEnabled() const;
    bool isRecording() const;

public slots:
    void setRecording(bool);
    virtual void clearData();
    virtual void sendRecordingStatus();

signals:
    void complete();
    void recordingChanged(bool arg);
    void enabledChanged();
    void cleared();

protected:
    virtual void stateChanged(State);

protected:
    bool m_recording;
    bool m_enabled;
};

class DeclarativeProfileClient : public ProfileClient
{
    Q_OBJECT

public:
    DeclarativeProfileClient(QDeclarativeDebugConnection *client);
    ~DeclarativeProfileClient();

public slots:
    void clearData();
    void sendRecordingStatus();

signals:
    void traceFinished( qint64 time );
    void traceStarted( qint64 time );
    void range(QDeclarativeProfilerService::RangeType type, qint64 startTime,
               qint64 length, const QStringList &data,
               const EventLocation &location);
    void frame(qint64 time, int frameRate, int animationCount);

protected:
    virtual void messageReceived(const QByteArray &);

private:
    class DeclarativeProfileClientPrivate *d;
};

class V8ProfileClient : public ProfileClient
{
    Q_OBJECT

public:
    enum Message {
        V8Entry,
        V8Complete,

        V8MaximumMessage
    };

    V8ProfileClient(QDeclarativeDebugConnection *client);
    ~V8ProfileClient();

public slots:
    void sendRecordingStatus();

signals:
    void range(int depth, const QString &function, const QString &filename,
               int lineNumber, double totalTime, double selfTime);

protected:
    virtual void messageReceived(const QByteArray &);
};

#endif // PROFILECLIENT_H
