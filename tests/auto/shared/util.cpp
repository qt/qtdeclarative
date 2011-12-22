/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "util.h"

#include <QtDeclarative/QDeclarativeComponent>
#include <QtDeclarative/QDeclarativeError>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtCore/QTextStream>

QDeclarativeDataTest *QDeclarativeDataTest::m_instance = 0;

QDeclarativeDataTest::QDeclarativeDataTest() :
#ifdef QT_TESTCASE_BUILDDIR
    m_dataDirectory(QTest::qFindTestData("data", QT_DECLARATIVETEST_DATADIR, 0, QT_TESTCASE_BUILDDIR)),
#else
    m_dataDirectory(QTest::qFindTestData("data", QT_DECLARATIVETEST_DATADIR, 0)),
#endif

    m_dataDirectoryUrl(QUrl::fromLocalFile(m_dataDirectory + QLatin1Char('/')))
{
    m_instance = this;
}

QDeclarativeDataTest::~QDeclarativeDataTest()
{
    m_instance = 0;
}

void QDeclarativeDataTest::initTestCase()
{
    QVERIFY2(!m_dataDirectory.isEmpty(), "'data' directory not found");
    m_directory = QFileInfo(m_dataDirectory).absolutePath();
    QVERIFY2(QDir::setCurrent(m_directory), qPrintable(QLatin1String("Could not chdir to ") + m_directory));
}

QString QDeclarativeDataTest::testFile(const QString &fileName) const
{
    if (m_directory.isEmpty())
        qFatal("QDeclarativeDataTest::initTestCase() not called.");
    QString result = m_dataDirectory;
    result += QLatin1Char('/');
    result += fileName;
    return result;
}

QByteArray QDeclarativeDataTest::msgComponentError(const QDeclarativeComponent &c,
                                                   const QDeclarativeEngine *engine /* = 0 */)
{
    QString result;
    const QList<QDeclarativeError> errors = c.errors();
    QTextStream str(&result);
    str << "Component '" << c.url().toString() << "' has " << errors.size()
        << " errors: '";
    for (int i = 0; i < errors.size(); ++i) {
        if (i)
            str << ", '";
        str << errors.at(i).toString() << '\'';

    }
    if (!engine)
        if (QDeclarativeContext *context = c.creationContext())
            engine = context->engine();
    if (engine) {
        str << " Import paths: (" << engine->importPathList().join(QStringLiteral(", "))
            << ") Plugin paths: (" << engine->pluginPathList().join(QStringLiteral(", "))
            << ')';
    }
    return result.toLocal8Bit();
}
