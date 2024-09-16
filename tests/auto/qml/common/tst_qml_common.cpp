// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qml_common.h"

using namespace Qt::StringLiterals;

void tst_qml_common::tst_propertyNameToChangedSignalName_data()
{
    QTest::addColumn<QString>("property");
    QTest::addColumn<QString>("expected");

    QTest::addRow("normalProperty") << u"helloWorld"_s << u"helloWorldChanged"_s;
    QTest::addRow("changedProperty") << u"changed"_s << u"changedChanged"_s;
    QTest::addRow("chängedProperty") << u"chänged"_s << u"chängedChanged"_s;
}
void tst_qml_common::tst_propertyNameToChangedSignalName()
{
    QFETCH(QString, property);
    QFETCH(QString, expected);

    QVERIFY(QQmlSignalNames::isChangedSignalName(expected));
    QCOMPARE(QQmlSignalNames::propertyNameToChangedSignalName(property), expected);
    QCOMPARE(QQmlSignalNames::changedSignalNameToPropertyName(expected).value(), property);
}

void tst_qml_common::tst_propertyNameToChangedHandlerName_data()
{
    QTest::addColumn<QString>("property");
    QTest::addColumn<QString>("expected");

    QTest::addRow("normalProperty") << u"helloWorld"_s << u"onHelloWorldChanged"_s;
    QTest::addRow("changedProperty") << u"changed"_s << u"onChangedChanged"_s;
    QTest::addRow("chängedProperty") << u"chänged"_s << u"onChängedChanged"_s;
    QTest::addRow("äProperty") << u"ä"_s << u"onÄChanged"_s;
    QTest::addRow("_Property") << u"_"_s << u"on_Changed"_s;
    QTest::addRow("___123aProperty") << u"___123a"_s << u"on___123AChanged"_s;
    QTest::addRow("___123Property") << u"___123"_s << u"on___123Changed"_s;
    QTest::addRow("AProperty") << u"A"_s << u"onAChanged"_s;
    QTest::addRow("_Property") << u"_"_s << u"on_Changed"_s;
    QTest::addRow("$Property") << u"$"_s << u"on$Changed"_s;
}
void tst_qml_common::tst_propertyNameToChangedHandlerName()
{
    QFETCH(QString, property);
    QFETCH(QString, expected);

    QVERIFY(QQmlSignalNames::isChangedHandlerName(expected));
    QCOMPARE(QQmlSignalNames::propertyNameToChangedHandlerName(property), expected);
    auto reverse = QQmlSignalNames::changedHandlerNameToPropertyName(expected);
    QVERIFY(reverse);
    QEXPECT_FAIL("AProperty",
                 "Cannot distinguish between property names starting with upper case"
                 " from properties starting with a lower case letter.",
                 Continue);
    QCOMPARE(reverse.value(), property);
}

void tst_qml_common::tst_signalNameToHandlerName_data()
{
    QTest::addColumn<QString>("signalName");
    QTest::addColumn<QString>("expected");

    QTest::addRow("normalProperty") << u"helloWorld"_s << u"onHelloWorld"_s;
    QTest::addRow("changedProperty") << u"changed"_s << u"onChanged"_s;
    QTest::addRow("chängedProperty") << u"chänged"_s << u"onChänged"_s;
    QTest::addRow("äProperty") << u"ä"_s << u"onÄ"_s;
    QTest::addRow("_Property") << u"_"_s << u"on_"_s;
    QTest::addRow("___123aProperty") << u"___123a"_s << u"on___123A"_s;
    QTest::addRow("___123Property") << u"___123"_s << u"on___123"_s;
    QTest::addRow("AProperty") << u"A"_s << u"onA"_s;
    QTest::addRow("_Property") << u"_"_s << u"on_"_s;
    QTest::addRow("$Property") << u"$"_s << u"on$"_s;
}

void tst_qml_common::tst_signalNameToHandlerName()
{
    QFETCH(QString, signalName);
    QFETCH(QString, expected);

    QVERIFY(QQmlSignalNames::isHandlerName(expected));
    QCOMPARE(QQmlSignalNames::signalNameToHandlerName(signalName), expected);

    auto result = QQmlSignalNames::handlerNameToSignalName(expected);
    QVERIFY(result.has_value());

    QEXPECT_FAIL("AProperty",
                 "Cannot distinguish between signal names starting with upper case"
                 " from signal names starting with a lower case letter.",
                 Continue);

    QCOMPARE(result.value(), signalName);
}

void tst_qml_common::tst_changedSignalNameToPropertyName_data()
{
    // only test when it should return nothing, see also tst_propertyNameToChangedSignalName.
    QTest::addColumn<QString>("changedSignalName");

    QTest::addRow("normalProperty") << u"helloWorld"_s;
    QTest::addRow("Changed") << u"Changed"_s;
    QTest::addRow("empty") << u""_s;
}

void tst_qml_common::tst_changedSignalNameToPropertyName()
{
    QFETCH(QString, changedSignalName);

    QVERIFY(!QQmlSignalNames::changedSignalNameToPropertyName(changedSignalName).has_value());
}

void tst_qml_common::tst_changedHandlerNameToPropertyName_data()
{
    // only test when it should return nothing, see also tst_propertyNameToChangedHandler.
    QTest::addColumn<QString>("changedHandler");

    QTest::addRow("normalProperty") << u"helloWorld"_s;
    QTest::addRow("Changed") << u"Changed"_s;
    QTest::addRow("empty") << u""_s;
    QTest::addRow("empty2") << u"onChanged"_s;
    QTest::addRow("on") << u"on"_s;
}
void tst_qml_common::tst_changedHandlerNameToPropertyName()
{
    QFETCH(QString, changedHandler);

    QVERIFY(!QQmlSignalNames::changedHandlerNameToPropertyName(changedHandler).has_value());
}

void tst_qml_common::tst_handlerNameToSignalName_data()
{
    // only test when it should return nothing, see also tst_signalNameToHandlerName.
    QTest::addColumn<QString>("handler");

    QTest::addRow("normalProperty") << u"helloWorld"_s;
    QTest::addRow("Changed") << u"Changed"_s;
    QTest::addRow("empty") << u""_s;
    QTest::addRow("on") << u"on"_s;
}
void tst_qml_common::tst_handlerNameToSignalName()
{
    QFETCH(QString, handler);

    QVERIFY(!QQmlSignalNames::handlerNameToSignalName(handler).has_value());
}

void tst_qml_common::tst_isChangedHandlerName_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<bool>("expected");

    QTest::addRow("normalProperty") << u"helloWorld"_s << false;
    QTest::addRow("Changed") << u"Changed"_s << false;
    QTest::addRow("empty") << u""_s << false;
    QTest::addRow("empty2") << u"onChanged"_s << false;
    QTest::addRow("on") << u"on"_s << false;
    QTest::addRow("on_Changed") << u"on_Changed"_s << true;
    QTest::addRow("on$Changed") << u"on$Changed"_s << true;
}
void tst_qml_common::tst_isChangedHandlerName()
{
    QFETCH(QString, name);
    QFETCH(bool, expected);

    QCOMPARE(QQmlSignalNames::isChangedHandlerName(name), expected);
}

QTEST_MAIN(tst_qml_common)
