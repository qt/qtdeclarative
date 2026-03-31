// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlpreviewhandler.h"
#include "qqmlpreviewservice.h"

#include <QtCore/qtimer.h>
#include <QtCore/qsettings.h>
#include <QtCore/qlibraryinfo.h>

#include <QtGui/qwindow.h>
#include <QtGui/qguiapplication.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickitem.h>

#include <private/qabstractanimation_p.h>
#include <private/qhighdpiscaling_p.h>

QT_BEGIN_NAMESPACE

QQmlPreviewHandler::QQmlPreviewHandler(QObject *parent) : QObject(parent)
{
    m_fpsTimer.setInterval(1000);
    connect(&m_fpsTimer, &QTimer::timeout, this, &QQmlPreviewHandler::fpsTimerHit);
}

QQmlPreviewHandler::~QQmlPreviewHandler() = default;

QQuickItem *QQmlPreviewHandler::currentRootItem() const
{
    return m_currentRootItem;
}

void QQmlPreviewHandler::setCurrentRootItem(QQuickItem *item)
{
    m_currentRootItem = item;
}

void QQmlPreviewHandler::addEngine(QQmlEngine *qmlEngine)
{
    m_engines.append(qmlEngine);
}

void QQmlPreviewHandler::removeEngine(QQmlEngine *qmlEngine)
{
    m_engines.removeOne(qmlEngine);
}

void QQmlPreviewHandler::setAnimationSpeed(qreal newFactor)
{
    QUnifiedTimer::instance()->setSpeedModifier(newFactor);
}

void QQmlPreviewHandler::zoomWindow(QQuickWindow *window, qreal zoomFactor,
                                    QQmlPreviewPosition *position)
{
    if (!window)
        return;
    if (qFuzzyIsNull(zoomFactor)) {
        emit error(QString::fromLatin1("Zooming with factor: %1 will result in nothing "
                                       "so it will be ignored.")
                           .arg(zoomFactor));
        return;
    }

    bool resetZoom = false;
    if (zoomFactor < 0) {
        resetZoom = true;
        zoomFactor = 1.0;
    }

    window->setGeometry(window->geometry());

    position->takePosition(window, QQmlPreviewPosition::InitializePosition);
    window->destroy();

    for (QScreen *screen : QGuiApplication::screens())
        QHighDpiScaling::setScreenFactor(screen, zoomFactor);
    if (resetZoom)
        QHighDpiScaling::updateHighDpiScaling();

    window->show();
    position->initLastSavedWindowPosition(window);
}

void QQmlPreviewHandler::disconnectWindow(QQuickWindow *window)
{
    disconnect(window, &QQuickWindow::beforeSynchronizing,
               this, &QQmlPreviewHandler::beforeSynchronizing);
    disconnect(window, &QQuickWindow::afterSynchronizing,
               this, &QQmlPreviewHandler::afterSynchronizing);
    disconnect(window, &QQuickWindow::beforeRendering,
               this, &QQmlPreviewHandler::beforeRendering);
    disconnect(window, &QQuickWindow::frameSwapped,
               this, &QQmlPreviewHandler::frameSwapped);
    m_fpsTimer.stop();
    m_rendering = FrameTime();
    m_synchronizing = FrameTime();
}

void QQmlPreviewHandler::connectWindow(QQuickWindow *window)
{
    connect(window, &QQuickWindow::beforeSynchronizing,
            this, &QQmlPreviewHandler::beforeSynchronizing, Qt::DirectConnection);
    connect(window, &QQuickWindow::afterSynchronizing,
            this, &QQmlPreviewHandler::afterSynchronizing, Qt::DirectConnection);
    connect(window, &QQuickWindow::beforeRendering,
            this, &QQmlPreviewHandler::beforeRendering, Qt::DirectConnection);
    connect(window, &QQuickWindow::frameSwapped,
            this, &QQmlPreviewHandler::frameSwapped, Qt::DirectConnection);
    m_fpsTimer.start();
}

void QQmlPreviewHandler::connectToService(QQmlPreviewServiceImpl *service)
{
    connect(service, &QQmlPreviewServiceImpl::load, this, &QQmlPreviewHandler::load);
    connect(service, &QQmlPreviewServiceImpl::animationSpeed,
            this, &QQmlPreviewHandler::setAnimationSpeed);
    connect(this, &QQmlPreviewHandler::error,
            service, &QQmlPreviewServiceImpl::forwardError, Qt::DirectConnection);
    connect(this, &QQmlPreviewHandler::fps,
            service, &QQmlPreviewServiceImpl::forwardFps, Qt::DirectConnection);
}

void QQmlPreviewHandler::beforeSynchronizing()
{
    m_synchronizing.beginFrame();
}

void QQmlPreviewHandler::afterSynchronizing()
{

    if (m_rendering.elapsed >= 0)
        m_rendering.endFrame();
    m_synchronizing.recordFrame();
    m_synchronizing.endFrame();
}

void QQmlPreviewHandler::beforeRendering()
{
    m_rendering.beginFrame();
}

void QQmlPreviewHandler::frameSwapped()
{
    m_rendering.recordFrame();
}

void QQmlPreviewHandler::FrameTime::beginFrame()
{
    timer.start();
}

void QQmlPreviewHandler::FrameTime::recordFrame()
{
    elapsed = timer.elapsed();
}

void QQmlPreviewHandler::FrameTime::endFrame()
{
    if (elapsed < min)
        min = static_cast<quint16>(qMax(0ll, elapsed));
    if (elapsed > max)
        max = static_cast<quint16>(qMin(qint64(std::numeric_limits<quint16>::max()), elapsed));
    total = static_cast<quint16>(qBound(0ll, qint64(std::numeric_limits<quint16>::max()),
                                        elapsed + total));
    ++number;
    elapsed = -1;
}

void QQmlPreviewHandler::FrameTime::reset()
{
    min = std::numeric_limits<quint16>::max();
    max = 0;
    total = 0;
    number = 0;
}

void QQmlPreviewHandler::fpsTimerHit()
{
    const FpsInfo info = {
        m_synchronizing.number,
        m_synchronizing.min,
        m_synchronizing.max,
        m_synchronizing.total,

        m_rendering.number,
        m_rendering.min,
        m_rendering.max,
        m_rendering.total
    };

    emit fps(info);

    m_rendering.reset();
    m_synchronizing.reset();
}

QQuickWindow *QQmlPreviewHandler::currentWindow() const
{
    return m_currentWindow.data();
}

void QQmlPreviewHandler::setCurrentWindow(QQuickWindow *window)
{
    if (window == m_currentWindow.data())
        return;

    if (m_currentWindow)
        disconnectWindow(m_currentWindow.data());

    m_currentWindow = window;

    if (m_currentWindow)
        connectWindow(m_currentWindow.data());
}


QT_END_NAMESPACE

#include "moc_qqmlpreviewhandler.cpp"
