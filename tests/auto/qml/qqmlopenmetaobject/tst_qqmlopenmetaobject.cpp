// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qqmlcontextdata_p.h>
#include <private/qqmldata_p.h>
#include <private/qqmlopenmetaobject_p.h>

#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>

class tst_qqmlopenmetaobject : public QObject
{
    Q_OBJECT
public:
    tst_qqmlopenmetaobject() {}

private slots:
    void createProperties();
    void setValue();
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

void tst_qqmlopenmetaobject::setValue()
{
    QQmlEngine engine;
    CustomObject object;
    const QQmlRefPointer<QQmlOpenMetaObjectType> mot(
            new QQmlOpenMetaObjectType(object.metaObject()),
            QQmlRefPointer<QQmlOpenMetaObjectType>::Adopt);
    QQmlOpenMetaObject *const mo = new QQmlOpenMetaObject(&object, mot);
    mo->setCached(true);
    const QQmlData *ddata = QQmlData::get(&object);
    QVERIFY(ddata);
    const QQmlPropertyCache::ConstPtr propCache = ddata->propertyCache;
    QVERIFY(propCache);
    mo->setValue("customProperty2", QVariant::fromValue<int>(25));
    QCOMPARE(QQmlData::get(&object), ddata);
    QCOMPARE(ddata->propertyCache, propCache);

    const QQmlPropertyData *propData
            = propCache->property(QLatin1String("customProperty2"), nullptr, nullptr);
    QVERIFY(propData);
    QCOMPARE(propData->propType(), QMetaType::fromType<QVariant>());
    QVariant result;
    propData->readProperty(&object, &result);
    QCOMPARE(result, QVariant::fromValue<int>(25));
}

QTEST_MAIN(tst_qqmlopenmetaobject)

#include "tst_qqmlopenmetaobject.moc"
