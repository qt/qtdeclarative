/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmlutils_p.h"

#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>

QT_BEGIN_NAMESPACE

QQmlDataTest *QQmlDataTest::m_instance = 0;

QQmlDataTest::QQmlDataTest(const char *qmlTestDataDir) :
    m_qmlTestDataDir(qmlTestDataDir),
#ifdef QT_TESTCASE_BUILDDIR
    m_dataDirectory(QTest::qFindTestData("data", m_qmlTestDataDir, 0, QT_TESTCASE_BUILDDIR)),
#else
    m_dataDirectory(QTest::qFindTestData("data", m_qmlTestDataDir, 0)),
#endif

    m_dataDirectoryUrl(m_dataDirectory.startsWith(QLatin1Char(':'))
        ? QUrl(QLatin1String("qrc") + m_dataDirectory)
        : QUrl::fromLocalFile(m_dataDirectory + QLatin1Char('/')))
{
    m_instance = this;
}

QQmlDataTest::~QQmlDataTest()
{
    m_instance = 0;
}

void QQmlDataTest::initTestCase()
{
    QVERIFY2(!m_dataDirectory.isEmpty(), qPrintable(QLatin1String(
        "'data' directory not found in ") + QFileInfo(QString::fromUtf8(m_qmlTestDataDir)).absolutePath()));
    m_directory = QFileInfo(m_dataDirectory).absolutePath();
    if (m_dataDirectoryUrl.scheme() != QLatin1String("qrc"))
        QVERIFY2(QDir::setCurrent(m_directory), qPrintable(QLatin1String("Could not chdir to ") + m_directory));
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

QQmlTestMessageHandler *QQmlTestMessageHandler::m_instance = 0;

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
    QQmlTestMessageHandler::m_instance = 0;
}

QT_END_NAMESPACE
