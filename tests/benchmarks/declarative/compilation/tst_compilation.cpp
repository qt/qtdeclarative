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

#include <qtest.h>

#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/private/qdeclarativejsengine_p.h>
#include <QtDeclarative/private/qdeclarativejsmemorypool_p.h>
#include <QtDeclarative/private/qdeclarativejsparser_p.h>
#include <QtDeclarative/private/qdeclarativejslexer_p.h>
#include <QtDeclarative/private/qdeclarativescript_p.h>

#include <QFile>
#include <QDebug>
#include <QTextStream>

class tst_compilation : public QObject
{
    Q_OBJECT
public:
    tst_compilation();

private slots:
    void boomblock();

    void jsparser_data();
    void jsparser();

    void scriptparser_data();
    void scriptparser();

private:
    QDeclarativeEngine engine;
};

tst_compilation::tst_compilation()
{
}

inline QUrl TEST_FILE(const QString &filename)
{
    return QUrl::fromLocalFile(QLatin1String(SRCDIR) + QLatin1String("/data/") + filename);
}

void tst_compilation::boomblock()
{
    QFile f(SRCDIR + QLatin1String("/data/BoomBlock.qml"));
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray data = f.readAll();

    //get rid of initialization effects
    {
        QDeclarativeComponent c(&engine);
        c.setData(data, QUrl());
    }

    QBENCHMARK {
        QDeclarativeComponent c(&engine);
        c.setData(data, QUrl());
//        QVERIFY(c.isReady());
    }
}

void tst_compilation::jsparser_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("boomblock") << QString(SRCDIR + QLatin1String("/data/BoomBlock.qml"));
}

void tst_compilation::jsparser()
{
    QFETCH(QString, file);

    QFile f(file);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray data = f.readAll();

    QTextStream stream(data, QIODevice::ReadOnly);
    const QString code = stream.readAll();

    QBENCHMARK {
        QDeclarativeJS::Engine engine;
        QDeclarativeJS::NodePool nodePool(file, &engine);

        QDeclarativeJS::Lexer lexer(&engine);
        lexer.setCode(code, -1);

        QDeclarativeJS::Parser parser(&engine);
        parser.parse();
        parser.ast();
    }
}

void tst_compilation::scriptparser_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("boomblock") << QString(SRCDIR + QLatin1String("/data/BoomBlock.qml"));
}

void tst_compilation::scriptparser()
{
    QFETCH(QString, file);

    QFile f(file);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray data = f.readAll();

    QUrl url = QUrl::fromLocalFile(file);

    QBENCHMARK {
        QDeclarativeScript::Parser parser;
        parser.parse(data, url);
        parser.tree();
    }
}

QTEST_MAIN(tst_compilation)

#include "tst_compilation.moc"
