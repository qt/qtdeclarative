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

#include "qqmlinspectorservicefactory.h"
#include "qquickviewinspector.h"

#include <private/qqmlglobal_p.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QPluginLoader>

QT_BEGIN_NAMESPACE

class QQmlInspectorServiceImpl : public QQmlInspectorService
{
    Q_OBJECT

public:
    QQmlInspectorServiceImpl(QObject *parent = 0);

    void addView(QObject *);
    void removeView(QObject *);

protected:
    virtual void stateChanged(State state);
    virtual void messageReceived(const QByteArray &);

private Q_SLOTS:
    void processMessage(const QByteArray &message);
    void updateState();

private:
    friend class QQmlInspectorServiceFactory;

    QList<QObject*> m_views;
    QmlJSDebugger::AbstractViewInspector *m_currentInspector;
};

QQmlInspectorServiceImpl::QQmlInspectorServiceImpl(QObject *parent):
    QQmlInspectorService(1, parent), m_currentInspector(0)
{
}

void QQmlInspectorServiceImpl::addView(QObject *view)
{
    m_views.append(view);
    updateState();
}

void QQmlInspectorServiceImpl::removeView(QObject *view)
{
    m_views.removeAll(view);
    updateState();
}

void QQmlInspectorServiceImpl::stateChanged(State /*state*/)
{
    QMetaObject::invokeMethod(this, "updateState", Qt::QueuedConnection);
}

void QQmlInspectorServiceImpl::updateState()
{
    delete m_currentInspector;
    m_currentInspector = 0;

    if (m_views.isEmpty() || state() != Enabled)
        return;

    QQuickView *qtQuickView = qobject_cast<QQuickView*>(m_views.first());
    if (qtQuickView)
        m_currentInspector = new QmlJSDebugger::QQuickViewInspector(this, qtQuickView, this);
    else
        qWarning() << "QQmlInspector: No inspector available for view '"
                   << m_views.first()->metaObject()->className() << "'.";
}

void QQmlInspectorServiceImpl::messageReceived(const QByteArray &message)
{
    QMetaObject::invokeMethod(this, "processMessage", Qt::QueuedConnection, Q_ARG(QByteArray, message));
}

void QQmlInspectorServiceImpl::processMessage(const QByteArray &message)
{
    if (m_currentInspector)
        m_currentInspector->handleMessage(message);
}

QQmlDebugService *QQmlInspectorServiceFactory::create(const QString &key)
{
    return key == QQmlInspectorServiceImpl::s_key ? new QQmlInspectorServiceImpl(this) : 0;
}

QT_END_NAMESPACE

#include "qqmlinspectorservice.moc"
