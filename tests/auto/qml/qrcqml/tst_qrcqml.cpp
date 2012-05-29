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
#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDebug>

// Loading QML files from embedded resources is a common QML usecase.
// This test just verifies that it works at a basic level, and with the qrc:/ syntax

class tst_qrcqml : public QObject
{
    Q_OBJECT
public:
    tst_qrcqml();

private slots:
    void basicLoad();
};

tst_qrcqml::tst_qrcqml()
{
}

void tst_qrcqml::basicLoad()
{
    QQmlEngine e;
    QQmlComponent c(&e, QUrl("qrc:/main.qml"));
    if (c.isError())
        qDebug() << "Error: " << c.errors();
    QEXPECT_FAIL("", "QTBUG-25937", Abort);
    QVERIFY(c.isReady());
    QObject* o = c.create();
    QVERIFY(o);
    QCOMPARE(o->property("tokenProperty").toString(), QLatin1String("hello"));
    delete o;
}
QTEST_MAIN(tst_qrcqml)

#include "tst_qrcqml.moc"
