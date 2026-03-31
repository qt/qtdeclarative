// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPREVIEWHANDLER_H
#define QQMLPREVIEWHANDLER_H

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

#include "qqmlpreviewposition.h"

#include <QtCore/qobject.h>
#include <QtCore/qvector.h>
#include <QtCore/qrect.h>
#include <QtCore/qpointer.h>
#include <QtCore/qelapsedtimer.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QQuickItem;
class QQuickWindow;
class QQmlPreviewServiceImpl;

class QQmlPreviewHandler : public QObject
{
    Q_OBJECT
public:
    struct FpsInfo {
        quint16 numSyncs;
        quint16 minSync;
        quint16 maxSync;
        quint16 totalSync;

        quint16 numRenders;
        quint16 minRender;
        quint16 maxRender;
        quint16 totalRender;
    };

    explicit QQmlPreviewHandler(QObject *parent = nullptr);
    ~QQmlPreviewHandler();

    QQuickItem *currentRootItem() const;
    void setCurrentRootItem(QQuickItem *item);

    QQuickWindow *currentWindow() const;
    void setCurrentWindow(QQuickWindow *window);

    virtual void connectToService(QQmlPreviewServiceImpl *service);
    virtual void addEngine(QQmlEngine *engine);
    virtual void removeEngine(QQmlEngine *engine);

    virtual void load(const QUrl &url) = 0;
    void setAnimationSpeed(qreal newFactor);

    QList<QQmlEngine *> engines() const { return m_engines; }

Q_SIGNALS:
    void error(const QString &message);
    void fps(const QQmlPreviewHandler::FpsInfo &info);

protected:
    struct FrameTime {
        void beginFrame();
        void recordFrame();
        void endFrame();
        void reset();

        QElapsedTimer timer;
        qint64 elapsed = -1;
        quint16 min = std::numeric_limits<quint16>::max();
        quint16 max = 0;
        quint16 total = 0;
        quint16 number = 0;
    };

    void connectWindow(QQuickWindow *window);
    void disconnectWindow(QQuickWindow *window);
    void zoomWindow(QQuickWindow *window, qreal zoomFactor, QQmlPreviewPosition *position);

private:
    void beforeSynchronizing();
    void afterSynchronizing();
    void beforeRendering();
    void frameSwapped();

    void fpsTimerHit();

    QList<QQmlEngine *> m_engines;

    QPointer<QQuickItem> m_currentRootItem;
    QPointer<QQuickWindow> m_currentWindow;

    QTimer m_fpsTimer;
    FrameTime m_rendering;
    FrameTime m_synchronizing;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlPreviewHandler::FpsInfo)

#endif // QQMLPREVIEWHANDLER_H
