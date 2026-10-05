// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtTest/qtest.h>

#include <memory>

using namespace Qt::StringLiterals;

class tst_QmlNoResourceTargetPath : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void aotLookupValidation();
};

void tst_QmlNoResourceTargetPath::initTestCase()
{
    qputenv("QV4_FAIL_ON_INVALID_AOT", "1");
}

void tst_QmlNoResourceTargetPath::aotLookupValidation()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/NtpThing.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);

    QVERIFY(QMetaObject::invokeMethod(object.get(), "poke"));
    QCOMPARE(object->property("burnerLife").toReal(), 1.0);
}

QTEST_MAIN(tst_QmlNoResourceTargetPath)

#include "tst_qmlnoresourcetargetpath.moc"
