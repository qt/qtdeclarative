// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant


#ifndef QQMLPREVIEWCLIENT_P_H
#define QQMLPREVIEWCLIENT_P_H

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

#include <private/qqmldebugclient_p.h>
#include <private/qqmldebugconnection_p.h>

#include <private/qqmlprofilerqtdwriter_p.h>
#include <private/qqmlprofilerclient_p.h>
#include <private/qquickeventreplayclient_p.h>

#include <QtCore/QTimer>

QT_BEGIN_NAMESPACE

class QQmlPreviewClientPrivate;
class QQmlPreviewClient : public QQmlDebugClient
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlPreviewClient)
public:
    enum Command {
        File,
        Load,
        Request,
        Error,
        Rerun,
        Directory,
        ClearCache,
        Zoom,
        Fps,
        AnimationSpeed,
        Configuration,
        Confirmation,
        HotReloadFailure,
    };

    struct Settings {
        bool enableInPlaceUpdates = false;
    };

    struct FpsInfo {
        quint16 numSyncs = 0;
        quint16 minSync = std::numeric_limits<quint16>::max();
        quint16 maxSync = 0;
        quint16 totalSync = 0;

        quint16 numRenders = 0;
        quint16 minRender = std::numeric_limits<quint16>::max();
        quint16 maxRender = 0;
        quint16 totalRender = 0;
    };

    QQmlPreviewClient(QQmlDebugConnection *parent);
    void messageReceived(const QByteArray &message) override;

    void sendDirectory(const QString &path, const QStringList &entries);
    void sendFile(const QString &path, const QByteArray &contents);
    void sendError(const QString &path);
    void sendConfiguration(const Settings &settings);

    void triggerLoad(const QUrl &url);
    void triggerRerun();
    void triggerZoom(float factor);
    void triggerAnimationSpeed(float factor);

    void replayEvents();
    void replayEvent(const QQmlProfilerEventType &type, QQmlProfilerEvent &&event);

    bool saveEvents(const QString &fileName);
    bool replayEventsFromFile(const QString &fileName);
    bool hasRecordedEvents() const;
    void clearRecordedEvents();

Q_SIGNALS:
    void request(const QString &path);
    void error(const QString &message);
    void fps(const QQmlPreviewClient::FpsInfo &info);
    void confirmation(const QQmlPreviewClient::Settings &settings);
    void hotReloadFailure(const QString &reason);

private:
    void configureEventReplay();
};

QT_END_NAMESPACE

#endif // QQMLPREVIEWCLIENT_P_H
