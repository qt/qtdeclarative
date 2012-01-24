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
#include <qsignalspy.h>
#include <private/qdeclarativeglobal_p.h>

class tst_qdeclarativecpputils : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativecpputils() {}

private slots:
    void fastConnect();
};

class MyObject : public QObject {
    Q_OBJECT
public:
    MyObject() : slotCount(0) {}
    friend class tst_qdeclarativecpputils;

    int slotCount;

signals:
    void signal1();
    void signal2();

public slots:
    void slot1() { slotCount++; }
};

void tst_qdeclarativecpputils::fastConnect()
{
    {
        MyObject *obj = new MyObject;
        FAST_CONNECT(obj, SIGNAL(signal1()), obj, SLOT(slot1()));

        obj->signal1();
        QCOMPARE(obj->slotCount, 1);

        delete obj;
    }

    {
        MyObject obj;
        FAST_CONNECT(&obj, SIGNAL(signal1()), &obj, SLOT(slot1()))

        obj.signal1();
        QCOMPARE(obj.slotCount, 1);
    }

    {
        MyObject *obj = new MyObject;
        QSignalSpy spy(obj, SIGNAL(signal2()));
        FAST_CONNECT(obj, SIGNAL(signal1()), obj, SIGNAL(signal2()));

        obj->signal1();
        QCOMPARE(spy.count(), 1);

        delete obj;
    }
}

QTEST_MAIN(tst_qdeclarativecpputils)

#include "tst_qdeclarativecpputils.moc"
