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
#include "globalinspector.h"
#include "qquickwindowinspector.h"

#include <QtGui/QWindow>

QT_BEGIN_NAMESPACE

class QQmlInspectorServiceImpl : public QQmlInspectorService
{
    Q_OBJECT
public:
    QQmlInspectorServiceImpl(QObject *parent = 0);

    void addWindow(QQuickWindow *window) Q_DECL_OVERRIDE;
    void setParentWindow(QQuickWindow *window, QWindow *parent) Q_DECL_OVERRIDE;
    void removeWindow(QQuickWindow *window) Q_DECL_OVERRIDE;

protected:
    virtual void messageReceived(const QByteArray &) Q_DECL_OVERRIDE;

private slots:
    void messageFromClient(const QByteArray &message);

private:
    friend class QQmlInspectorServiceFactory;

    QmlJSDebugger::GlobalInspector *checkInspector();
    QmlJSDebugger::GlobalInspector *m_globalInspector;
    QHash<QQuickWindow *, QWindow *> m_waitingWindows;
};

QQmlInspectorServiceImpl::QQmlInspectorServiceImpl(QObject *parent):
    QQmlInspectorService(1, parent), m_globalInspector(0)
{
}

QmlJSDebugger::GlobalInspector *QQmlInspectorServiceImpl::checkInspector()
{
    if (state() == Enabled) {
        if (!m_globalInspector) {
            m_globalInspector = new QmlJSDebugger::GlobalInspector(this);
            connect(m_globalInspector, &QmlJSDebugger::GlobalInspector::messageToClient,
                    this, &QQmlDebugService::messageToClient);
            for (QHash<QQuickWindow *, QWindow *>::ConstIterator i = m_waitingWindows.constBegin();
                 i != m_waitingWindows.constEnd(); ++i) {
                m_globalInspector->addWindow(i.key());
                if (i.value() != 0)
                    m_globalInspector->setParentWindow(i.key(), i.value());
            }
            m_waitingWindows.clear();
        }
    } else if (m_globalInspector) {
        delete m_globalInspector;
        m_globalInspector = 0;
    }
    return m_globalInspector;
}

void QQmlInspectorServiceImpl::addWindow(QQuickWindow *window)
{
    if (QmlJSDebugger::GlobalInspector *inspector = checkInspector())
        inspector->addWindow(window);
    else
        m_waitingWindows.insert(window, 0);
}

void QQmlInspectorServiceImpl::removeWindow(QQuickWindow *window)
{
    if (QmlJSDebugger::GlobalInspector *inspector = checkInspector())
        inspector->removeWindow(window);
    else
        m_waitingWindows.remove(window);
}

void QQmlInspectorServiceImpl::setParentWindow(QQuickWindow *window, QWindow *parent)
{
    if (QmlJSDebugger::GlobalInspector *inspector = checkInspector())
        inspector->setParentWindow(window, parent);
    else
        m_waitingWindows[window] = parent;
}

void QQmlInspectorServiceImpl::messageReceived(const QByteArray &message)
{
    QMetaObject::invokeMethod(this, "messageFromClient", Qt::QueuedConnection,
                              Q_ARG(QByteArray, message));
}

void QQmlInspectorServiceImpl::messageFromClient(const QByteArray &message)
{
    if (QmlJSDebugger::GlobalInspector *inspector = checkInspector())
        inspector->processMessage(message);
}

QQmlDebugService *QQmlInspectorServiceFactory::create(const QString &key)
{
    return key == QQmlInspectorServiceImpl::s_key ? new QQmlInspectorServiceImpl(this) : 0;
}

QT_END_NAMESPACE

#include "qqmlinspectorservice.moc"
