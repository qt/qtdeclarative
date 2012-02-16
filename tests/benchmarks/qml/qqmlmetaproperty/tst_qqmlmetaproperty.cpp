/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlProperty>
#include <QFile>
#include <QDebug>

class tst_qmlmetaproperty : public QObject
{
    Q_OBJECT

public:
    tst_qmlmetaproperty();
    virtual ~tst_qmlmetaproperty();

public slots:
    void init();
    void cleanup();

private slots:
    void lookup_data();
    void lookup();

private:
    QQmlEngine engine;
};

tst_qmlmetaproperty::tst_qmlmetaproperty()
{
}

tst_qmlmetaproperty::~tst_qmlmetaproperty()
{
}

void tst_qmlmetaproperty::init()
{
}

void tst_qmlmetaproperty::cleanup()
{
}

void tst_qmlmetaproperty::lookup_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("Simple Object") << SRCDIR "/data/object.qml";
    QTest::newRow("Synthesized Object") << SRCDIR "/data/synthesized_object.qml";
}

void tst_qmlmetaproperty::lookup()
{
    QFETCH(QString, file);

    QQmlComponent c(&engine, file);
    QVERIFY(c.isReady());

    QObject *obj = c.create();

    QBENCHMARK {
        QQmlProperty p(obj, "x");
    }

    delete obj;
}

QTEST_MAIN(tst_qmlmetaproperty)
#include "tst_qqmlmetaproperty.moc"
