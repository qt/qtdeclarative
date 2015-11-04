/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickprofiler_p.h"

#include <QtQml/private/qqmlabstractprofileradapter_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

// instance will be set, unset in constructor. Allows static methods to be inlined.
QQuickProfiler *QQuickProfiler::s_instance = 0;
quint64 QQuickProfiler::featuresEnabled = 0;

void QQuickProfiler::initialize(QObject *parent)
{
    Q_ASSERT(s_instance == 0);
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
public slots:
    void registerAnimationTimerCallback()
    {
        QQuickProfiler::registerAnimationCallback();
        delete this;
    }
};

#include "qquickprofiler.moc"

QQuickProfiler::QQuickProfiler(QObject *parent) : QObject(parent)
{
    // This is safe because at this point the m_instance isn't initialized, yet.
    m_timer.start();
    CallbackRegistrationHelper *helper = new CallbackRegistrationHelper; // will delete itself
    helper->moveToThread(QCoreApplication::instance()->thread());
    QMetaObject::invokeMethod(helper, "registerAnimationTimerCallback", Qt::QueuedConnection);
}

QQuickProfiler::~QQuickProfiler()
{
    QMutexLocker lock(&m_dataMutex);
    featuresEnabled = 0;
    s_instance = 0;
}

void QQuickProfiler::startProfilingImpl(quint64 features)
{
    QMutexLocker lock(&m_dataMutex);
    m_data.clear();
    featuresEnabled = features;
}

void QQuickProfiler::stopProfilingImpl()
{
    {
        QMutexLocker lock(&m_dataMutex);
        featuresEnabled = 0;
    }
    emit dataReady(m_data);
}

void QQuickProfiler::reportDataImpl()
{
    emit dataReady(m_data);
}

void QQuickProfiler::setTimer(const QElapsedTimer &t)
{
    QMutexLocker lock(&m_dataMutex);
    m_timer = t;
}

QT_END_NAMESPACE
