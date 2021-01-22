/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/


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
        Language
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

    void triggerLoad(const QUrl &url);
    void triggerRerun();
    void triggerZoom(float factor);
    void triggerLanguage(const QUrl &url, const QString &locale);

signals:
    void request(const QString &path);
    void error(const QString &message);
    void fps(const FpsInfo &info);
};

QT_END_NAMESPACE

#endif // QQMLPREVIEWCLIENT_P_H
