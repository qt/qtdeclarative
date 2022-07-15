// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLPREVIEWSERVICE_H
#define QQMLPREVIEWSERVICE_H

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

#include "qqmlpreviewhandler.h"
#include "qqmlpreviewfileengine.h"
#include <private/qqmldebugserviceinterfaces_p.h>

QT_BEGIN_NAMESPACE

class QQmlPreviewFileEngineHandler;
class QQmlPreviewHandler;
class QQmlPreviewServiceImpl : public QQmlDebugService
{
    Q_OBJECT

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
        Fps
    };

    static const QString s_key;

    QQmlPreviewServiceImpl(QObject *parent = nullptr);
    virtual ~QQmlPreviewServiceImpl();

    void messageReceived(const QByteArray &message) override;
    void engineAboutToBeAdded(QJSEngine *engine) override;
    void engineAboutToBeRemoved(QJSEngine *engine) override;
    void stateChanged(State state) override;

    void forwardRequest(const QString &file);
    void forwardError(const QString &error);
    void forwardFps(const QQmlPreviewHandler::FpsInfo &frames);

    QQuickItem *currentRootItem();

Q_SIGNALS:
    void error(const QString &file);
    void file(const QString &file, const QByteArray &contents);
    void directory(const QString &file, const QStringList &entries);
    void load(const QUrl &url);
    void rerun();
    void clearCache();
    void zoom(qreal factor);

private:
    QScopedPointer<QQmlPreviewFileEngineHandler> m_fileEngine;
    QScopedPointer<QQmlPreviewFileLoader> m_loader;
    QQmlPreviewHandler m_handler;
    QUrl m_currentUrl;
};

QT_END_NAMESPACE

#endif // QQMLPREVIEWSERVICE_H
