// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <private/qqmlopenmetaobject_p.h>
#include <QtQml/qqmlengine.h>

class tst_qqmlopenmetaobject : public QObject
{
    Q_OBJECT
public:
    tst_qqmlopenmetaobject() {}

private slots:
    void createProperties();
};

class CustomObject: public QObject
{
    Q_OBJECT
public:
    CustomObject(QObject *parent = nullptr)
        : QObject(parent) {}
};

void tst_qqmlopenmetaobject::createProperties()
{
    QQmlEngine engine;
    CustomObject object;
    const QQmlRefPointer<QQmlOpenMetaObjectType> mot(
                new QQmlOpenMetaObjectType(object.metaObject()),
                QQmlRefPointer<QQmlOpenMetaObjectType>::Adopt);
    QQmlOpenMetaObject *const mo = new QQmlOpenMetaObject(&object, mot.data());
    mo->setCached(true);
    mot->createProperty("customProperty");
    QVERIFY(true);
}

QTEST_MAIN(tst_qqmlopenmetaobject)

#include "tst_qqmlopenmetaobject.moc"
