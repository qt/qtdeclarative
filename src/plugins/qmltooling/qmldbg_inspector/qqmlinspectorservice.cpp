// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qqmldebugservicefactory_p.h>

#include "globalinspector.h"
#include "qquickwindowinspector.h"

#include <QtGui/QWindow>

QT_BEGIN_NAMESPACE

class QQmlInspectorServiceImpl : public QQmlInspectorService
{
    Q_OBJECT
public:
    QQmlInspectorServiceImpl(QObject *parent = nullptr);

    void addWindow(QQuickWindow *window) override;
    void setParentWindow(QQuickWindow *window, QWindow *parent) override;
    void removeWindow(QQuickWindow *window) override;

signals:
    void scheduleMessage(const QByteArray &message);

protected:
    void messageReceived(const QByteArray &) override;

private:
    friend class QQmlInspectorServiceFactory;

    QmlJSDebugger::GlobalInspector *checkInspector();
    QmlJSDebugger::GlobalInspector *m_globalInspector;
    QHash<QQuickWindow *, QWindow *> m_waitingWindows;

    void messageFromClient(const QByteArray &message);
};

QQmlInspectorServiceImpl::QQmlInspectorServiceImpl(QObject *parent):
    QQmlInspectorService(1, parent), m_globalInspector(nullptr)
{
    connect(this, &QQmlInspectorServiceImpl::scheduleMessage,
            this, &QQmlInspectorServiceImpl::messageFromClient, Qt::QueuedConnection);
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
        m_globalInspector = nullptr;
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
    // Move the message to the right thread via queued signal
    emit scheduleMessage(message);
}

void QQmlInspectorServiceImpl::messageFromClient(const QByteArray &message)
{
    if (QmlJSDebugger::GlobalInspector *inspector = checkInspector())
        inspector->processMessage(message);
}

class QQmlInspectorServiceFactory : public QQmlDebugServiceFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlDebugServiceFactory_iid FILE "qqmlinspectorservice.json")
public:
    QQmlDebugService *create(const QString &key) override;
};

QQmlDebugService *QQmlInspectorServiceFactory::create(const QString &key)
{
    return key == QQmlInspectorServiceImpl::s_key ? new QQmlInspectorServiceImpl(this) : nullptr;
}

QT_END_NAMESPACE

#include "qqmlinspectorservice.moc"
