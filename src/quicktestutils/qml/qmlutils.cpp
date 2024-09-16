// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmlutils_p.h"

#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

QQmlDataTest *QQmlDataTest::m_instance = nullptr;

QQmlDataTest::QQmlDataTest(
        const char *qmlTestDataDir, FailOnWarningsPolicy failOnWarningsPolicy,
        const char *dataSubDir)
    : m_qmlTestDataDir(qmlTestDataDir)
#ifdef QT_TESTCASE_BUILDDIR
    , m_dataDirectory(QTest::qFindTestData(dataSubDir, m_qmlTestDataDir, 0, QT_TESTCASE_BUILDDIR))
#else
    , m_dataDirectory(QTest::qFindTestData(dataSubDir, m_qmlTestDataDir, 0))
#endif
    , m_dataDirectoryUrl(m_dataDirectory.startsWith(QLatin1Char(':'))
                         ? QUrl(QLatin1String("qrc") + m_dataDirectory + QLatin1Char('/'))
                         : QUrl::fromLocalFile(m_dataDirectory + QLatin1Char('/')))
    , m_failOnWarningsPolicy(failOnWarningsPolicy)
{
    m_instance = this;
    if (m_cacheDir.isValid() && !qEnvironmentVariableIsSet("QML_DISK_CACHE_PATH")) {
        m_usesOwnCacheDir = true;
        qputenv("QML_DISK_CACHE_PATH", m_cacheDir.path().toLocal8Bit());
    }
}

QQmlDataTest::~QQmlDataTest()
{
    m_instance = nullptr;
    if (m_usesOwnCacheDir)
        qunsetenv("QML_DISK_CACHE_PATH");
}

void QQmlDataTest::initTestCase()
{
    QVERIFY2(!m_dataDirectory.isEmpty(), qPrintable(
                 QLatin1String("'%1' directory not found in %2").arg(
                     QString::fromUtf8(m_dataSubDir),
                     QFileInfo(QString::fromUtf8(m_qmlTestDataDir)).absolutePath())));
    m_directory = QFileInfo(m_dataDirectory).absolutePath();
    if (m_dataDirectoryUrl.scheme() != QLatin1String("qrc"))
        QVERIFY2(QDir::setCurrent(m_directory), qPrintable(QLatin1String("Could not chdir to ") + m_directory));
}

void QQmlDataTest::init()
{
    if (m_failOnWarningsPolicy == FailOnWarningsPolicy::FailOnWarnings)
        QTest::failOnWarning(QRegularExpression(QStringLiteral(".?")));
}

QString QQmlDataTest::testFile(const QString &fileName) const
{
    if (m_directory.isEmpty())
        qFatal("QQmlDataTest::initTestCase() not called.");
    QString result = m_dataDirectory;
    result += QLatin1Char('/');
    result += fileName;
    return result;
}

bool QQmlDataTest::canImportModule(const QString &importTestQmlSource) const
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(importTestQmlSource.toLatin1(), QUrl());
    return !component.isError();
}

Q_GLOBAL_STATIC(QMutex, qQmlTestMessageHandlerMutex)

QQmlTestMessageHandler *QQmlTestMessageHandler::m_instance = nullptr;

void QQmlTestMessageHandler::messageHandler(QtMsgType, const QMessageLogContext &context, const QString &message)
{
    QMutexLocker locker(qQmlTestMessageHandlerMutex());
    if (QQmlTestMessageHandler::m_instance) {
        if (QQmlTestMessageHandler::m_instance->m_includeCategories) {
            QQmlTestMessageHandler::m_instance->m_messages.push_back(
                QString::fromLatin1("%1: %2").arg(QString::fromUtf8(context.category), message));
        } else {
            QQmlTestMessageHandler::m_instance->m_messages.push_back(message);
        }
    }
}

QQmlTestMessageHandler::QQmlTestMessageHandler()
{
    QMutexLocker locker(qQmlTestMessageHandlerMutex());
    Q_ASSERT(!QQmlTestMessageHandler::m_instance);
    QQmlTestMessageHandler::m_instance = this;
    m_oldHandler = qInstallMessageHandler(messageHandler);
    m_includeCategories = false;
}

QQmlTestMessageHandler::~QQmlTestMessageHandler()
{
    QMutexLocker locker(qQmlTestMessageHandlerMutex());
    Q_ASSERT(QQmlTestMessageHandler::m_instance);
    qInstallMessageHandler(m_oldHandler);
    QQmlTestMessageHandler::m_instance = nullptr;
}


bool gcDone(const QV4::ExecutionEngine *engine) {
    return !engine->memoryManager->gcStateMachine->inProgress();
}

void gc(QV4::ExecutionEngine &engine, GCFlags flags)
{
    engine.memoryManager->runGC();
    while (!gcDone(&engine))
        engine.memoryManager->gcStateMachine->step();
    if (int(GCFlags::DontSendPostedEvents) & int(flags))
        return;
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

bool gcDone(QQmlEngine *engine) {
    auto priv = QQmlEnginePrivate::get(engine);
    return gcDone(priv->v4engine());
}

void gc(QQmlEngine &engine, GCFlags flags)
{
    auto priv = QQmlEnginePrivate::get(&engine);
    gc(*priv->v4engine(), flags);
}


QT_END_NAMESPACE

#include "moc_qmlutils_p.cpp"
