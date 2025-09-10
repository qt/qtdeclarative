// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickprofiler_p.h"

#include <QtQml/private/qqmlabstractprofileradapter_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

// instance will be set, unset in constructor. Allows static methods to be inlined.
QQuickProfiler *QQuickProfiler::s_instance = nullptr;
quint64 QQuickProfiler::featuresEnabled = 0;

void QQuickProfiler::initialize(QObject *parent)
{
    Q_ASSERT(s_instance == nullptr);
    s_instance = new QQuickProfiler(parent);
}

void animationTimerCallback(qint64 delta)
{
    Q_QUICK_PROFILE(QQuickProfiler::ProfileAnimations, animationFrame(delta,
            QThread::currentThread() == QCoreApplication::instance()->thread() ?
            QQuickProfiler::GuiThread : QQuickProfiler::RenderThread));
}

void QQuickProfiler::registerAnimationCallback()
{
    QUnifiedTimer::instance()->registerProfilerCallback(&animationTimerCallback);
}

class CallbackRegistrationHelper : public QObject {
    Q_OBJECT
public:
    void registerAnimationTimerCallback()
    {
        QQuickProfiler::registerAnimationCallback();
        delete this;
    }
};

QQuickProfiler::QQuickProfiler(QObject *parent) : QObject(parent)
{
    // This is safe because at this point the m_instance isn't initialized, yet.
    m_timer.start();
    CallbackRegistrationHelper *helper = new CallbackRegistrationHelper; // will delete itself
    helper->moveToThread(QCoreApplication::instance()->thread());

    // Queue the signal to have the animation timer registration run in the right thread;
    QObject signalSource;
    connect(&signalSource, &QObject::destroyed,
            helper, &CallbackRegistrationHelper::registerAnimationTimerCallback,
            Qt::QueuedConnection);
}

QQuickProfiler::~QQuickProfiler()
{
    QMutexLocker lock(&m_dataMutex);
    featuresEnabled = 0;
    s_instance = nullptr;
}

void QQuickProfiler::startProfilingImpl(quint64 features)
{
    QMutexLocker lock(&m_dataMutex);
    featuresEnabled = features;
}

void QQuickProfiler::stopProfilingImpl()
{
    QMutexLocker lock(&m_dataMutex);
    featuresEnabled = 0;
    emit dataReady(m_data);
    m_data.clear();
}

void QQuickProfiler::reportDataImpl()
{
    QMutexLocker lock(&m_dataMutex);
    emit dataReady(m_data);
    m_data.clear();
}

void QQuickProfiler::setTimer(const QElapsedTimer &t)
{
    QMutexLocker lock(&m_dataMutex);
    m_timer = t;
}

QT_END_NAMESPACE

#include "qquickprofiler.moc"
#include "moc_qquickprofiler_p.cpp"
