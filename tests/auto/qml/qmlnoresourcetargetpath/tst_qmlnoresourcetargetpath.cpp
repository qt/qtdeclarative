// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest/qtest.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(NtpVersionedPlugin)

using namespace Qt::StringLiterals;

class tst_QmlNoResourceTargetPath : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void aotLookupValidation_data();
    void aotLookupValidation();
};

void tst_QmlNoResourceTargetPath::initTestCase()
{
    qputenv("QV4_FAIL_ON_INVALID_AOT", "1");
}

void tst_QmlNoResourceTargetPath::aotLookupValidation_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QByteArray>("lifeProperty");

    QTest::newRow("sameModule") << QUrl(u"qrc:/NtpThing.qml"_s) << "burnerLife"_ba;
    QTest::newRow("qualifiedOnly") << QUrl(u"qrc:/NtpQualifiedOnly.qml"_s) << "lateLife"_ba;
    QTest::newRow("mixedVersions") << QUrl(u"qrc:/NtpMixedVersions.qml"_s) << "lateLife"_ba;
    QTest::newRow("shifting1") << QUrl(u"qrc:/NtpShifting1.qml"_s) << "shiftingLife"_ba;
    QTest::newRow("shifting2") << QUrl(u"qrc:/NtpShifting2.qml"_s) << "shiftingLife"_ba;
}

void tst_QmlNoResourceTargetPath::aotLookupValidation()
{
    QFETCH(QUrl, url);
    QFETCH(QByteArray, lifeProperty);

    QQmlEngine engine;
    QQmlComponent component(&engine, url);
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);

    QVERIFY(QMetaObject::invokeMethod(object.get(), "poke"));
    QCOMPARE(object->property(lifeProperty.constData()).toReal(), 1.0);
}

QTEST_MAIN(tst_QmlNoResourceTargetPath)

#include "tst_qmlnoresourcetargetpath.moc"
