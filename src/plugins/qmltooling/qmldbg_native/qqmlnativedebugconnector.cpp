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

#include <private/qqmldebugconnector_p.h>
#include <private/qhooks_p.h>

#include <qqmlengine.h>

#include <QtCore/qdebug.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qpointer.h>
#include <QtCore/qvector.h>

//#define TRACE_PROTOCOL(s) qDebug() << s
#define TRACE_PROTOCOL(s)

QT_USE_NAMESPACE

static bool expectSyncronousResponse = false;
Q_GLOBAL_STATIC(QByteArray, responseBuffer)

extern "C" {

Q_DECL_EXPORT const char *qt_qmlDebugMessageBuffer;
Q_DECL_EXPORT int qt_qmlDebugMessageLength;
Q_DECL_EXPORT bool qt_qmlDebugConnectionBlocker;

// In blocking mode, this will busy wait until the debugger sets block to false.
Q_DECL_EXPORT void qt_qmlDebugConnectorOpen();

// First thing, set the debug stream version. Please use this function as we might move the version
// member to some other place.
Q_DECL_EXPORT void qt_qmlDebugSetStreamVersion(int version)
{
    QQmlDebugStream::s_dataStreamVersion = version;
}


// Break in this one to process output from an asynchronous message/
Q_DECL_EXPORT void qt_qmlDebugMessageAvailable()
{
}


// Break in this one to get notified about construction and destruction of
// interesting objects, such as QmlEngines.
Q_DECL_EXPORT void qt_qmlDebugObjectAvailable()
{
}

Q_DECL_EXPORT void qt_qmlDebugClearBuffer()
{
    responseBuffer->clear();
}

// Send a message to a service.
Q_DECL_EXPORT bool qt_qmlDebugSendDataToService(const char *serviceName, const char *hexData)
{
    QByteArray msg = QByteArray::fromHex(hexData);

    QQmlDebugConnector *instance = QQmlDebugConnector::instance();
    if (!instance)
        return false;

    QQmlDebugService *recipient = instance->service(serviceName);
    if (!recipient)
        return false;

    TRACE_PROTOCOL("Recipient: " << recipient << " got message: " << msg);
    expectSyncronousResponse = true;
    recipient->messageReceived(msg);
    expectSyncronousResponse = false;

    return true;
}

// Enable a service.
Q_DECL_EXPORT bool qt_qmlDebugEnableService(const char *data)
{
    QQmlDebugConnector *instance = QQmlDebugConnector::instance();
    if (!instance)
        return false;

    QString name = QString::fromLatin1(data);
    QQmlDebugService *service = instance->service(name);
    if (!service || service->state() == QQmlDebugService::Enabled)
        return false;

    service->stateAboutToBeChanged(QQmlDebugService::Enabled);
    service->setState(QQmlDebugService::Enabled);
    service->stateChanged(QQmlDebugService::Enabled);
    return true;
}

Q_DECL_EXPORT bool qt_qmlDebugDisableService(const char *data)
{
    QQmlDebugConnector *instance = QQmlDebugConnector::instance();
    if (!instance)
        return false;

    QString name = QString::fromLatin1(data);
    QQmlDebugService *service = instance->service(name);
    if (!service || service->state() == QQmlDebugService::Unavailable)
        return false;

    service->stateAboutToBeChanged(QQmlDebugService::Unavailable);
    service->setState(QQmlDebugService::Unavailable);
    service->stateChanged(QQmlDebugService::Unavailable);
    return true;
}

quintptr qt_qmlDebugTestHooks[] = {
    quintptr(1), // Internal Version
    quintptr(6), // Number of entries following
    quintptr(&qt_qmlDebugMessageBuffer),
    quintptr(&qt_qmlDebugMessageLength),
    quintptr(&qt_qmlDebugSendDataToService),
    quintptr(&qt_qmlDebugEnableService),
    quintptr(&qt_qmlDebugDisableService),
    quintptr(&qt_qmlDebugObjectAvailable)
};

// In blocking mode, this will busy wait until the debugger sets block to false.
Q_DECL_EXPORT void qt_qmlDebugConnectorOpen()
{
    TRACE_PROTOCOL("Opening native debug connector");

    // FIXME: Use a dedicated hook. Startup is a safe workaround, though,
    // as we are already beyond its only use.
    qtHookData[QHooks::Startup] = quintptr(&qt_qmlDebugTestHooks);

    while (qt_qmlDebugConnectionBlocker)
        ;

    TRACE_PROTOCOL("Opened native debug connector");
}

} // extern "C"

QT_BEGIN_NAMESPACE

class QQmlNativeDebugConnector : public QQmlDebugConnector
{
    Q_OBJECT

public:
    QQmlNativeDebugConnector();
    ~QQmlNativeDebugConnector();

    bool blockingMode() const;
    QQmlDebugService *service(const QString &name) const;
    void addEngine(QQmlEngine *engine);
    void removeEngine(QQmlEngine *engine);
    bool addService(const QString &name, QQmlDebugService *service);
    bool removeService(const QString &name);
    bool open(const QVariantHash &configuration);

private slots:
    void sendMessage(const QString &name, const QByteArray &message);
    void sendMessages(const QString &name, const QList<QByteArray> &messages);

private:
    void announceObjectAvailability(const QString &objectType, QObject *object, bool available);

    QVector<QQmlDebugService *> m_services;
    bool m_blockingMode;
};

QQmlNativeDebugConnector::QQmlNativeDebugConnector()
    : m_blockingMode(false)
{
    const QString args = commandLineArguments();
    const QStringList lstjsDebugArguments = args.split(QLatin1Char(','));
    QStringList services;
    QStringList::const_iterator argsItEnd = lstjsDebugArguments.cend();
    QStringList::const_iterator argsIt = lstjsDebugArguments.cbegin();
    for (; argsIt != argsItEnd; ++argsIt) {
        const QString strArgument = *argsIt;
        if (strArgument == QLatin1String("block")) {
            m_blockingMode = true;
        } else if (strArgument == QLatin1String("native")) {
            // Ignore. This is used to signal that this connector
            // should be loaded and that has already happened.
        } else if (strArgument.startsWith(QLatin1String("services:"))) {
            services.append(strArgument.mid(9));
        } else if (!services.isEmpty()) {
            services.append(strArgument);
        } else {
            qWarning("QML Debugger: Invalid argument \"%s\" detected. Ignoring the same.",
                     qUtf8Printable(strArgument));
        }
    }
    setServices(services);
}

QQmlNativeDebugConnector::~QQmlNativeDebugConnector()
{
    foreach (QQmlDebugService *service, m_services) {
        service->stateAboutToBeChanged(QQmlDebugService::NotConnected);
        service->setState(QQmlDebugService::NotConnected);
        service->stateChanged(QQmlDebugService::NotConnected);
    }
}

bool QQmlNativeDebugConnector::blockingMode() const
{
    return m_blockingMode;
}

QQmlDebugService *QQmlNativeDebugConnector::service(const QString &name) const
{
    for (QVector<QQmlDebugService *>::ConstIterator i = m_services.begin(); i != m_services.end();
         ++i) {
        if ((*i)->name() == name)
            return *i;
    }
    return 0;
}

void QQmlNativeDebugConnector::addEngine(QQmlEngine *engine)
{
    TRACE_PROTOCOL("Add engine to connector:" << engine);
    foreach (QQmlDebugService *service, m_services)
        service->engineAboutToBeAdded(engine);

    announceObjectAvailability(QLatin1String("qmlengine"), engine, true);

    foreach (QQmlDebugService *service, m_services)
        service->engineAdded(engine);
}

void QQmlNativeDebugConnector::removeEngine(QQmlEngine *engine)
{
    TRACE_PROTOCOL("Remove engine from connector:" << engine);
    foreach (QQmlDebugService *service, m_services)
        service->engineAboutToBeRemoved(engine);

    announceObjectAvailability(QLatin1String("qmlengine"), engine, false);

    foreach (QQmlDebugService *service, m_services)
        service->engineRemoved(engine);
}

void QQmlNativeDebugConnector::announceObjectAvailability(const QString &objectType,
                                                          QObject *object, bool available)
{
    QJsonObject ob;
    ob.insert(QLatin1String("objecttype"), objectType);
    ob.insert(QLatin1String("object"), QString::number(quintptr(object)));
    ob.insert(QLatin1String("available"), available);
    QJsonDocument doc;
    doc.setObject(ob);

    QByteArray ba = doc.toJson(QJsonDocument::Compact);
    qt_qmlDebugMessageBuffer = ba.constData();
    qt_qmlDebugMessageLength = ba.size();
    TRACE_PROTOCOL("Reporting engine availabilty");
    qt_qmlDebugObjectAvailable(); // Trigger native breakpoint.
}

bool QQmlNativeDebugConnector::addService(const QString &name, QQmlDebugService *service)
{
    TRACE_PROTOCOL("Add service to connector: " << qPrintable(name) << service);
    for (QVector<QQmlDebugService *>::ConstIterator i = m_services.begin(); i != m_services.end();
         ++i) {
        if ((*i)->name() == name)
            return false;
    }

    connect(service, &QQmlDebugService::messageToClient,
            this, &QQmlNativeDebugConnector::sendMessage);
    connect(service, &QQmlDebugService::messagesToClient,
            this, &QQmlNativeDebugConnector::sendMessages);

    service->setState(QQmlDebugService::Unavailable);

    m_services << service;
    return true;
}

bool QQmlNativeDebugConnector::removeService(const QString &name)
{
    for (QVector<QQmlDebugService *>::Iterator i = m_services.begin(); i != m_services.end(); ++i) {
        if ((*i)->name() == name) {
            QQmlDebugService *service = *i;
            m_services.erase(i);
            service->setState(QQmlDebugService::NotConnected);

            disconnect(service, &QQmlDebugService::messagesToClient,
                       this, &QQmlNativeDebugConnector::sendMessages);
            disconnect(service, &QQmlDebugService::messageToClient,
                       this, &QQmlNativeDebugConnector::sendMessage);

            return true;
        }
    }
    return false;
}

bool QQmlNativeDebugConnector::open(const QVariantHash &configuration)
{
    m_blockingMode = configuration.value(QStringLiteral("block"), m_blockingMode).toBool();
    qt_qmlDebugConnectionBlocker = m_blockingMode;
    qt_qmlDebugConnectorOpen();
    return true;
}

void QQmlNativeDebugConnector::sendMessage(const QString &name, const QByteArray &message)
{
    (*responseBuffer) += name.toUtf8() + ' ' + QByteArray::number(message.size()) + ' ' + message;
    qt_qmlDebugMessageBuffer = responseBuffer->constData();
    qt_qmlDebugMessageLength = responseBuffer->size();
    // Responses are allowed to accumulate, the buffer will be cleared by
    // separate calls to qt_qmlDebugClearBuffer() once the synchronous
    // function return ('if' branch below) or in the native breakpoint handler
    // ('else' branch below).
    if (expectSyncronousResponse) {
        TRACE_PROTOCOL("Expected synchronous response in " << message);
        // Do not trigger the native breakpoint on qt_qmlDebugMessageFromService.
    } else {
        TRACE_PROTOCOL("Found asynchronous message in " << message);
        // Trigger native breakpoint.
        qt_qmlDebugMessageAvailable();
    }
}

void QQmlNativeDebugConnector::sendMessages(const QString &name, const QList<QByteArray> &messages)
{
    for (int i = 0; i != messages.size(); ++i)
        sendMessage(name, messages.at(i));
}

class QQmlNativeDebugConnectorFactory : public QQmlDebugConnectorFactory
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID QQmlDebugConnectorFactory_iid FILE "qqmlnativedebugconnector.json")

public:
    QQmlNativeDebugConnectorFactory() {}

    QQmlDebugConnector *create(const QString &key)
    {
        return key == QLatin1String("QQmlNativeDebugConnector") ? new QQmlNativeDebugConnector : 0;
    }
};

QT_END_NAMESPACE

#include "qqmlnativedebugconnector.moc"
