// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <qsignalspy.h>
#include <private/qqmlglobal_p.h>

class tst_qqmlcpputils : public QObject
{
    Q_OBJECT
public:
    tst_qqmlcpputils() {}

private slots:
    void fastConnect();
    void fastCast();
};

class MyObject : public QObject {
    Q_OBJECT
public:
    MyObject() : slotCount(0) {}
    friend class tst_qqmlcpputils;

    int slotCount;

signals:
    void signal1();
    void signal2();

public slots:
    void slot1() { slotCount++; }
};

void tst_qqmlcpputils::fastConnect()
{
    {
        MyObject *obj = new MyObject;
        qmlobject_connect(obj, MyObject, SIGNAL(signal1()), obj, MyObject, SLOT(slot1()));

        obj->signal1();
        QCOMPARE(obj->slotCount, 1);

        delete obj;
    }

    {
        MyObject obj;
        qmlobject_connect(&obj, MyObject, SIGNAL(signal1()), &obj, MyObject, SLOT(slot1()));

        obj.signal1();
        QCOMPARE(obj.slotCount, 1);
    }

    {
        MyObject *obj = new MyObject;
        QSignalSpy spy(obj, SIGNAL(signal2()));
        qmlobject_connect(obj, MyObject, SIGNAL(signal1()), obj, MyObject, SIGNAL(signal2()));

        obj->signal1();
        QCOMPARE(spy.size(), 1);

        delete obj;
    }
}

void tst_qqmlcpputils::fastCast()
{
    {
        QObject *myObj = new MyObject;
        MyObject *obj = qmlobject_cast<MyObject*>(myObj);
        QVERIFY(obj);
        QCOMPARE(obj->metaObject(), myObj->metaObject());
        obj->slot1();
        QCOMPARE(obj->slotCount, 1);
        delete myObj;
    }

    {
        QObject *nullObj = nullptr;
        QObject *obj = qmlobject_cast<QObject *>(nullObj);
        QCOMPARE(obj, nullObj); // shouldn't crash/assert.
    }
}

QTEST_MAIN(tst_qqmlcpputils)

#include "tst_qqmlcpputils.moc"
