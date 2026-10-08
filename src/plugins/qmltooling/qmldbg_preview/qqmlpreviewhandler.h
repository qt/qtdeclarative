// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
class QQmlPreviewUrlInterceptor;
class QQuickWindow;

class QQmlPreviewHandler : public QObject
{
    Q_OBJECT
public:
    explicit QQmlPreviewHandler(QObject *parent = nullptr);
    ~QQmlPreviewHandler();

    QQuickItem *currentRootItem();

    void addEngine(QQmlEngine *engine);
    void removeEngine(QQmlEngine *engine);

    void loadUrl(const QUrl &url);
    void dropCU(const QUrl &url);
    void rerun();
    void zoom(qreal newFactor);

    void clear();

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

Q_SIGNALS:
    void error(const QString &message);
    void fps(const FpsInfo &info);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    void doZoom();
    void tryCreateObject();
    void showObject(QObject *object);
    void setCurrentWindow(QQuickWindow *window);

    void beforeSynchronizing();
    void afterSynchronizing();
    void beforeRendering();
    void frameSwapped();

    void fpsTimerHit();

    QScopedPointer<QQuickItem> m_dummyItem;
    QList<QQmlEngine *> m_engines;
    QPointer<QQuickItem> m_currentRootItem;
    QVector<QPointer<QObject>> m_createdObjects;
    QScopedPointer<QQmlComponent> m_component;
    QPointer<QQuickWindow> m_currentWindow;
    qreal m_zoomFactor = 1.0;
    bool m_supportsMultipleWindows;
    QQmlPreviewPosition m_lastPosition;

    QTimer m_fpsTimer;

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

    FrameTime m_rendering;
    FrameTime m_synchronizing;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlPreviewHandler::FpsInfo)

#endif // QQMLPREVIEWHANDLER_H
