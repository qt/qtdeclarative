// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    const auto start = qmltypesData.indexOf("name: \"hiddenRead\""); // rely on name being 1st field
    QVERIFY(start != -1);
    const auto end = qmltypesData.indexOf("}", start); // enclosing '}' of hiddenRead property
    QVERIFY(end != -1);
    QVERIFY(start < end);

    const auto hiddenReadData = QByteArrayView(qmltypesData).sliced(start, end - start);
    // QVERIFY(hiddenReadData.contains("name: \"hiddenRead\"")); // tested above by start != -1
    QVERIFY(hiddenReadData.contains("type: \"QString\""));
    QVERIFY(hiddenReadData.contains("read: \"hiddenRead\""));
    QVERIFY(hiddenReadData.contains("privateClass: \"HiddenAccessorsPrivate\""));
    QVERIFY(hiddenReadData.contains("isReadonly: true"));
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

void tst_qmltyperegistrar::namespaceExtendedNamespace()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QtQml\n"
              "import QmlTypeRegistrarTest\n"
              "QtObject {\n"
              "    property int b: ForeignNamespace.B\n"
              "    property int f: ForeignNamespace.F\n"
              "}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("b").toInt(), int(ExtensionValueType::B));
    QCOMPARE(o->property("f").toInt(), int(BaseNamespace::F));
}

void tst_qmltyperegistrar::deferredNames()
{
    QVERIFY(qmltypesData.contains("deferredNames: [\"\"]"));
    QVERIFY(qmltypesData.contains("deferredNames: [\"A\", \"B\", \"C\"]"));
}

void tst_qmltyperegistrar::immediateNames()
{
    QVERIFY(qmltypesData.contains("immediateNames: [\"\"]"));
    QVERIFY(qmltypesData.contains("immediateNames: [\"A\", \"B\", \"C\"]"));
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

void tst_qmltyperegistrar::addRemoveVersion_data()
{
    QTest::addColumn<QTypeRevision>("importVersion");
    for (int i = 0; i < 20; ++i)
        QTest::addRow("v1.%d.qml", i) << QTypeRevision::fromVersion(1, i);
}

void tst_qmltyperegistrar::addRemoveVersion()
{
    QFETCH(QTypeRevision, importVersion);

    const bool creatable
            = importVersion > QTypeRevision::fromVersion(1, 2)
            && importVersion < QTypeRevision::fromVersion(1, 18);
    const bool thingAccessible = importVersion > QTypeRevision::fromVersion(1, 3);

    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData(QStringLiteral("import QmlTypeRegistrarTest %1.%2\n"
                             "Versioned {\n"
                             "    property int thing: revisioned\n"
                             "}")
              .arg(importVersion.majorVersion()).arg(importVersion.minorVersion()).toUtf8(),
              QUrl(QTest::currentDataTag()));
    if (!creatable) {
        QVERIFY(c.isError());
        return;
    }
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    if (!thingAccessible) {
        QTest::ignoreMessage(
                    QtWarningMsg,
                    qPrintable(QStringLiteral("%1:3: ReferenceError: revisioned is not defined")
                               .arg(QTest::currentDataTag())));
    }
    QScopedPointer o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("thing").toInt(), thingAccessible ? 24 : 0);
}

#ifdef QT_QUICK_LIB
void tst_qmltyperegistrar::foreignRevisionedProperty()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QmlTypeRegistrarTest\n"
              "ForeignRevisionedProperty {\n"
              "    activeFocusOnTab: true\n"
              "}",
              QUrl());

    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer o(c.create());
    QVERIFY(!o.isNull());
}
#endif

void tst_qmltyperegistrar::typeInModuleMajorVersionZero()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData(QStringLiteral("import VersionZero\n"
                             "TypeInModuleMajorVersionZero {}\n").toUtf8(),
              QUrl(QTest::currentDataTag()));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
}

QTEST_MAIN(tst_qmltyperegistrar)
