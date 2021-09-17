/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "tst_qmltyperegistrar.h"
#include <QtTest/qtest.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>

#include "hppheader.hpp"

void tst_qmltyperegistrar::initTestCase()
{
    Q_ASSERT(QCoreApplication::instance());
    QFile file(QCoreApplication::applicationDirPath() + "/tst_qmltyperegistrar.qmltypes");
    QVERIFY(file.open(QIODevice::ReadOnly));
    qmltypesData = file.readAll();
    QVERIFY(file.atEnd());
    QCOMPARE(file.error(), QFile::NoError);
}

void tst_qmltyperegistrar::qmltypesHasForeign()
{
    QVERIFY(qmltypesData.contains("things"));
}

void tst_qmltyperegistrar::qmltypesHasHppClassAndNoext()
{
    QVERIFY(qmltypesData.contains("HppClass"));
    QVERIFY(qmltypesData.contains("Noext"));
}

void tst_qmltyperegistrar::qmltypesHasReadAndWrite()
{
    QVERIFY(qmltypesData.contains(R"(read: "eieiei")"));
    QVERIFY(qmltypesData.contains(R"(write: "setEieiei")"));
}

void tst_qmltyperegistrar::qmltypesHasNotify()
{
    QVERIFY(qmltypesData.contains(R"(notify: "eieieiChanged")"));
}

void tst_qmltyperegistrar::qmltypesHasPropertyIndex()
{
    qsizetype start = qmltypesData.indexOf("notify: \"eieieiChanged\"");
    qsizetype end = qmltypesData.indexOf("}", start);
    // [start, end) - range in which index information of eieiei should exist
    QVERIFY(qmltypesData.indexOf("index: 0", start) < end); // belongs to eieiei

    start = qmltypesData.indexOf("read: \"eieiei2\"");
    end = qmltypesData.indexOf("}", start);
    QVERIFY(qmltypesData.indexOf("index: 1", start) < end); // belongs to eieiei2

    HppClass eieieiClass;
    const QMetaObject *mo = eieieiClass.metaObject();
    QVERIFY(mo);
    // NB: add 0 and 1 as relative indices "parsed" from qmltypesData
    QCOMPARE(mo->indexOfProperty("eieiei"), mo->propertyOffset() + 0);
    QCOMPARE(mo->indexOfProperty("eieiei2"), mo->propertyOffset() + 1);
}

void tst_qmltyperegistrar::qmltypesHasFileNames()
{
    QVERIFY(qmltypesData.contains("file: \"hppheader.hpp\""));
    QVERIFY(qmltypesData.contains("file: \"noextheader\""));
    QVERIFY(qmltypesData.contains("file: \"tst_qmltyperegistrar.h\""));
}

void tst_qmltyperegistrar::qmltypesHasFlags()
{
    QVERIFY(qmltypesData.contains("name: \"Flags\""));
    QVERIFY(qmltypesData.contains("alias: \"Flag\""));
    QVERIFY(qmltypesData.contains("isFlag: true"));
}

void tst_qmltyperegistrar::superAndForeignTypes()
{
    QVERIFY(qmltypesData.contains("values: [\"Pixel\", \"Centimeter\", \"Inch\", \"Point\"]"));
    QVERIFY(qmltypesData.contains("name: \"SizeGadget\""));
    QVERIFY(qmltypesData.contains("prototype: \"SizeEnums\""));
    QVERIFY(qmltypesData.contains("Property { name: \"height\"; type: \"int\"; read: \"height\"; write: \"setHeight\"; index: 0; isFinal: true }"));
    QVERIFY(qmltypesData.contains("Property { name: \"width\"; type: \"int\"; read: \"width\"; write: \"setWidth\"; index: 0; isFinal: true }"));
    QVERIFY(qmltypesData.contains("Method { name: \"sizeToString\"; type: \"QString\" }"));
    QCOMPARE(qmltypesData.count("extension: \"SizeValueType\""), 1);
}

void tst_qmltyperegistrar::accessSemantics()
{
    QVERIFY(qmltypesData.contains("accessSemantics: \"reference\""));
    QVERIFY(qmltypesData.contains("accessSemantics: \"value\""));
}

void tst_qmltyperegistrar::isBindable()
{
    QVERIFY(qmltypesData.contains(R"(Property { name: "someProperty"; type: "int"; bindable: "bindableSomeProperty"; index: 0 })"));
}

void tst_qmltyperegistrar::restrictToImportVersion()
{
    QVERIFY(qmltypesData.contains("ExcessiveVersion"));
    QVERIFY(!qmltypesData.contains("1536"));           // Q_REVISION(6, 0)
    QVERIFY(!qmltypesData.contains("paletteChanged")); // Added in version 6.0
}

void tst_qmltyperegistrar::pastMajorVersions()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QML\nimport QmlTypeRegistrarTest 0.254\nQtObject {}", QUrl());
    QVERIFY2(!c.isError(), qPrintable(c.errorString()));
}

void tst_qmltyperegistrar::implementsInterfaces()
{
    QVERIFY(qmltypesData.contains("interfaces: [\"Interface\"]"));
    QVERIFY(qmltypesData.contains("interfaces: [\"Interface\", \"Interface2\"]"));
}

void tst_qmltyperegistrar::namespacedElement()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QML\nimport QmlTypeRegistrarTest 1.0\nElement {}", QUrl());
    QVERIFY2(!c.isError(), qPrintable(c.errorString()));
}

void tst_qmltyperegistrar::derivedFromForeign()
{
    QVERIFY(qmltypesData.contains("name: \"DerivedFromForeign\""));
    QVERIFY(qmltypesData.contains("prototype: \"QTimeLine\""));
    QVERIFY(qmltypesData.contains("name: \"QTimeLine\""));
}

void tst_qmltyperegistrar::metaTypesRegistered()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QmlTypeRegistrarTest\nOoo {}", QUrl());
    QVERIFY(c.isReady());
    QScopedPointer<QObject> obj(c.create());

    auto verifyMetaType = [](const char *name, const char *className) {
        const auto foundMetaType = QMetaType::fromName(name);
        QVERIFY(foundMetaType.isValid());
        QCOMPARE(foundMetaType.name(), name);
        QVERIFY(foundMetaType.metaObject());
        QCOMPARE(foundMetaType.metaObject()->className(), className);
    };

    verifyMetaType("Foo", "Foo");
    verifyMetaType("Ooo*", "Ooo");
    verifyMetaType("Bbb*", "Bbb");
    verifyMetaType("Ccc*", "Ccc");
    verifyMetaType("SelfExtensionHack", "SelfExtensionHack");
}

void tst_qmltyperegistrar::multiExtensions()
{
    QVERIFY(qmltypesData.contains("name: \"MultiExtension\""));
    QVERIFY(qmltypesData.contains("prototype: \"MultiExtensionParent\""));
    QVERIFY(qmltypesData.contains("name: \"MultiExtensionParent\""));
    QVERIFY(qmltypesData.contains("extension: \"ExtensionA\""));
    QVERIFY(qmltypesData.contains("extension: \"ExtensionB\""));
    QVERIFY(qmltypesData.contains("interfaces: [\"Interface3\"]"));
}

void tst_qmltyperegistrar::localDefault()
{
    QQmlEngine engine;
    {
        QQmlComponent c(&engine);
        c.setData("import QmlTypeRegistrarTest\n"
                  "import QtQml\n"
                  "ForeignWithoutDefault { QtObject {} }", QUrl());
        QVERIFY(c.isError());
        QVERIFY(c.errorString().contains(
                    QStringLiteral("Cannot assign to non-existent default property")));
    }
    {
        QQmlComponent c(&engine);
        c.setData("import QmlTypeRegistrarTest\n"
                  "import QtQml\n"
                  "Local { QtObject {} }", QUrl());
        QVERIFY(c.isReady());
    }

    QCOMPARE(qmltypesData.count("name: \"LocalWithDefault\""), 1);
    QCOMPARE(qmltypesData.count("name: \"ForeignWithoutDefault\""), 1);
    QCOMPARE(qmltypesData.count("defaultProperty: \"d\""), 1);

    const int local = qmltypesData.indexOf("name: \"LocalWithDefault\"");
    const int foreign = qmltypesData.indexOf("name: \"ForeignWithoutDefault\"");
    const int defaultProp = qmltypesData.indexOf("defaultProperty: \"d\"");

    // We assume that name is emitted before defaultProperty.
    // Then this proves that the default property does not belong to ForeignWithoutDefault.
    QVERIFY(local < defaultProp);
    QVERIFY(foreign > defaultProp || foreign < local);
}

void tst_qmltyperegistrar::requiredProperty()
{
    QCOMPARE(qmltypesData.count("name: \"RequiredProperty\""), 1);
    QCOMPARE(qmltypesData.count("isRequired: true"), 1);
}

void tst_qmltyperegistrar::hiddenAccessor()
{
    // No read accessor
    QVERIFY(qmltypesData.contains(
                "Property { name: \"hiddenRead\"; type: \"QString\"; isReadonly: true }"));
}

void tst_qmltyperegistrar::finalProperty()
{
    QCOMPARE(qmltypesData.count("name: \"FinalProperty\""), 1);
    QCOMPARE(qmltypesData.count(
                     "Property { name: \"fff\"; type: \"int\"; index: 0; isFinal: true }"),
             1);
}

void tst_qmltyperegistrar::parentProperty()
{
    QCOMPARE(qmltypesData.count("parentProperty: \"ppp\""), 1);
}

void tst_qmltyperegistrar::namespacesAndValueTypes()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QmlTypeRegistrarTest\nLocal {}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer o(c.create());
    QVERIFY(!o.isNull());

    auto check = [&](QMetaType m1, QMetaType m2) {
        QVERIFY(m1.isValid());
        QVERIFY(m2.isValid());

        // Does not actually help if we have two types with equal IDs. It only compares the IDs.
        QVERIFY(m1 == m2);
        QCOMPARE(m1.id(), m2.id());

        // If we had a bogus namespace value type, it wouldn't be able to create the type.
        void *v1 = m1.create();
        QVERIFY(v1 != nullptr);
        m1.destroy(v1);

        void *v2 = m2.create();
        QVERIFY(v2 != nullptr);
        m2.destroy(v2);

        QMetaType m3(m1.id());
        QVERIFY(m3.isValid());
        void *v3 = m3.create();
        QVERIFY(v3 != nullptr);
        m3.destroy(v3);
    };

    check(QMetaType::fromName("ValueTypeWithEnum1"), QMetaType::fromType<ValueTypeWithEnum1>());
    check(QMetaType::fromName("ValueTypeWithEnum2"), QMetaType::fromType<ValueTypeWithEnum2>());
}

void tst_qmltyperegistrar::derivedFromForeignPrivate()
{
    QVERIFY(qmltypesData.contains("file: \"private/foreign_p.h\""));
}

void tst_qmltyperegistrar::methodReturnType()
{
    QVERIFY(qmltypesData.contains("createAThing"));
    QVERIFY(!qmltypesData.contains("QQmlComponent*"));
    QVERIFY(qmltypesData.contains("type: \"QQmlComponent\""));
}

QTEST_MAIN(tst_qmltyperegistrar)
