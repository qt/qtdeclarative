// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qmltyperegistrar.h"
#include <QtTest/qtest.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/qqmlprivate.h>

#include "hppheader.hpp"
#include "UnregisteredTypes/uncreatable.h"

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

void tst_qmltyperegistrar::resettableProperty()
{
    QVERIFY(qmltypesData.contains(R"(reset: "resetFoo")"));
}

void tst_qmltyperegistrar::duplicateExportWarnings()
{
    QmlTypeRegistrar r;
    QString moduleName = "tstmodule";
    QString targetNamespace = "tstnamespace";
    r.setModuleNameAndNamespace(moduleName, targetNamespace);

    MetaTypesJsonProcessor processor(true);
    QVERIFY(processor.processTypes({ ":/duplicatedExports.json" }));
    processor.postProcessTypes();
    QVector<QJsonObject> types = processor.types();
    QVector<QJsonObject> typesforeign = processor.foreignTypes();
    r.setTypes(types, typesforeign);

    auto expectWarning = [](QString message) {
        QTest::ignoreMessage(QtWarningMsg, qPrintable(message));
    };
    expectWarning("Warning: ExportedQmlElement was registered multiple times by following Cpp "
                  "classes:  ExportedQmlElement, ExportedQmlElement2 (added in 1.2), "
                  "ExportedQmlElementDifferentVersion (added in 1.0) (removed in 1.7)");
    expectWarning("Warning: SameNameSameExport was registered multiple times by following Cpp "
                  "classes:  SameNameSameExport, SameNameSameExport2 (added in 1.2), "
                  "SameNameSameExportDifferentVersion (added in 1.0)");

    QString outputData;
    QTextStream output(&outputData, QIODeviceBase::ReadWrite);
    r.write(output);
}

void tst_qmltyperegistrar::clonedSignal()
{
    QVERIFY(qmltypesData.contains(R"(Signal {
            name: "clonedSignal"
            Parameter { name: "i"; type: "int" }
        })"));

    QVERIFY(qmltypesData.contains(R"(Signal { name: "clonedSignal"; isCloned: true })"));
}

void tst_qmltyperegistrar::hasIsConstantInParameters()
{
    QVERIFY(qmltypesData.contains(R"(        Signal {
            name: "mySignal"
            Parameter { name: "myObject"; type: "QObject"; isPointer: true }
            Parameter { name: "myConstObject"; type: "QObject"; isPointer: true; isConstant: true }
            Parameter { name: "myConstObject2"; type: "QObject"; isPointer: true; isConstant: true }
            Parameter { name: "myObject2"; type: "QObject"; isPointer: true }
            Parameter { name: "myConstObject3"; type: "QObject"; isPointer: true; isConstant: true }
        }
)"));

    QVERIFY(qmltypesData.contains(R"(Signal {
            name: "myVolatileSignal"
            Parameter { name: "a"; type: "volatile QObject"; isPointer: true; isConstant: true }
            Parameter { name: "b"; type: "volatile QObject"; isPointer: true; isConstant: true }
            Parameter { name: "nonConst"; type: "volatile QObject"; isPointer: true }
        }
)"));
}

void tst_qmltyperegistrar::uncreatable()
{
    using namespace QQmlPrivate;

    // "normal" constructible types
    QVERIFY(QmlMetaType<Creatable>::hasAcceptableCtors());
    QVERIFY(QmlMetaType<Creatable2>::hasAcceptableCtors());

    // good singletons
    QCOMPARE((singletonConstructionMode<SingletonCreatable, SingletonCreatable>()),
             SingletonConstructionMode::Factory);
    QCOMPARE((singletonConstructionMode<SingletonCreatable2, SingletonCreatable2>()),
             SingletonConstructionMode::Constructor);
    QCOMPARE((singletonConstructionMode<SingletonCreatable2, SingletonCreatable2>()),
             SingletonConstructionMode::Constructor);
    QCOMPARE((singletonConstructionMode<SingletonForeign, SingletonLocalCreatable>()),
             SingletonConstructionMode::FactoryWrapper);

    // bad singletons
    QCOMPARE((singletonConstructionMode<SingletonIncreatable, SingletonIncreatable>()),
             SingletonConstructionMode::None);
    QCOMPARE((singletonConstructionMode<SingletonIncreatable2, SingletonIncreatable2>()),
             SingletonConstructionMode::None);
    QCOMPARE((singletonConstructionMode<SingletonIncreatable3, SingletonIncreatable3>()),
             SingletonConstructionMode::None);
    QCOMPARE((singletonConstructionMode<SingletonIncreatable4, SingletonIncreatable4>()),
             SingletonConstructionMode::None);
    QCOMPARE((singletonConstructionMode<SingletonIncreatableExtended,
                                        SingletonIncreatableExtended>()),
             SingletonConstructionMode::None);
    QCOMPARE((singletonConstructionMode<SingletonForeign, SingletonLocalUncreatable1>()),
             SingletonConstructionMode::None);
    QCOMPARE((singletonConstructionMode<SingletonForeign, SingletonLocalUncreatable2>()),
             SingletonConstructionMode::None);
#if QT_DEPRECATED_SINCE(6, 4)
    QTest::ignoreMessage(
                QtWarningMsg,
                "Singleton SingletonIncreatable needs either a default constructor or, "
                "when adding a default constructor is infeasible, a public static "
                "create(QQmlEngine *, QJSEngine *) method.");
    qmlRegisterTypesAndRevisions<SingletonIncreatable>("A", 1);
    QTest::ignoreMessage(
                QtWarningMsg,
                "Singleton SingletonIncreatable2 needs either a default constructor or, "
                "when adding a default constructor is infeasible, a public static "
                "create(QQmlEngine *, QJSEngine *) method.");
    qmlRegisterTypesAndRevisions<SingletonIncreatable2>("A", 1);
    QTest::ignoreMessage(
                QtWarningMsg,
                "Singleton SingletonIncreatable3 needs either a default constructor or, "
                "when adding a default constructor is infeasible, a public static "
                "create(QQmlEngine *, QJSEngine *) method.");
    qmlRegisterTypesAndRevisions<SingletonIncreatable3>("A", 1);
    QTest::ignoreMessage(
                QtWarningMsg,
                "Singleton SingletonIncreatable4 needs either a default constructor or, "
                "when adding a default constructor is infeasible, a public static "
                "create(QQmlEngine *, QJSEngine *) method.");
    qmlRegisterTypesAndRevisions<SingletonIncreatable4>("A", 1);
    QTest::ignoreMessage(
                QtWarningMsg,
                "Singleton SingletonIncreatableExtended needs either a default constructor or, "
                "when adding a default constructor is infeasible, a public static "
                "create(QQmlEngine *, QJSEngine *) method.");
    qmlRegisterTypesAndRevisions<SingletonIncreatableExtended>("A", 1);
    QTest::ignoreMessage(
                QtWarningMsg,
                "Singleton SingletonForeign needs either a default constructor or, "
                "when adding a default constructor is infeasible, a public static "
                "create(QQmlEngine *, QJSEngine *) method.");
    qmlRegisterTypesAndRevisions<SingletonLocalUncreatable1>("A", 1);
    QTest::ignoreMessage(
                QtWarningMsg,
                "Singleton SingletonForeign needs either a default constructor or, "
                "when adding a default constructor is infeasible, a public static "
                "create(QQmlEngine *, QJSEngine *) method.");
    qmlRegisterTypesAndRevisions<SingletonLocalUncreatable2>("A", 1);
#endif

    // QML_UNCREATABLE types
    QVERIFY(!QmlMetaType<BadUncreatable>::hasAcceptableCtors());
    QVERIFY(!QmlMetaType<BadUncreatableExtended>::hasAcceptableCtors());
    QVERIFY(!QmlMetaType<GoodUncreatable>::hasAcceptableCtors());
    QVERIFY(!QmlMetaType<UncreatableNeedsForeign>::hasAcceptableCtors());
    QVERIFY(!QmlMetaType<GoodUncreatableExtended>::hasAcceptableCtors());
#if QT_DEPRECATED_SINCE(6, 4)
    QTest::ignoreMessage(
                QtWarningMsg,
                "BadUncreatable is neither a QObject, nor default- and copy-constructible, "
                "nor uncreatable. You should not use it as a QML type.");
    qmlRegisterTypesAndRevisions<BadUncreatable>("A", 1);
    QTest::ignoreMessage(
                QtWarningMsg,
                "BadUncreatableExtended is neither a QObject, nor default- and copy-constructible, "
                "nor uncreatable. You should not use it as a QML type.");
    qmlRegisterTypesAndRevisions<BadUncreatableExtended>("A", 1);
#endif

    const auto oldHandler = qInstallMessageHandler(
                [](QtMsgType, const QMessageLogContext &, const QString &message) {
        QFAIL(qPrintable(message));
    });
    const auto guard = qScopeGuard([oldHandler](){qInstallMessageHandler(oldHandler); });

    // These should not print any messages.

    qmlRegisterTypesAndRevisions<Creatable>("A", 1);
    qmlRegisterTypesAndRevisions<Creatable2>("A", 1);

    qmlRegisterTypesAndRevisions<SingletonCreatable>("A", 1);
    qmlRegisterTypesAndRevisions<SingletonCreatable2>("A", 1);
    qmlRegisterTypesAndRevisions<SingletonCreatable3>("A", 1);

    qmlRegisterTypesAndRevisions<GoodUncreatable>("A", 1);
    qmlRegisterTypesAndRevisions<GoodUncreatable2>("A", 1);
    qmlRegisterTypesAndRevisions<GoodUncreatableExtended>("A", 1);
}

void tst_qmltyperegistrar::singletonVesions()
{
    QQmlEngine engine;
    qmlRegisterTypesAndRevisions<SingletonVesion0>("A", 0);
    qmlRegisterTypesAndRevisions<SingletonVesion1>("B", 1);

    QQmlComponent c(&engine);
    c.setData("import QtQuick\n"
              "import A\n"
              "import B\n"
              "QtObject {\n"
              "    property QtObject v0: SingletonVesion0\n"
              "    property QtObject v1: SingletonVesion1\n"
              "}", QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    auto obj = c.create();
    QVERIFY2(!obj->property("v0").isNull(), "Singleton version 0 is not registered");
    QVERIFY2(!obj->property("v1").isNull(), "Singleton version 1 is not registered");
}

void tst_qmltyperegistrar::baseVersionInQmltypes()
{
    // Since it has no QML_ADDED_IN_VERSION, WithMethod was added in .0 of the current version.
    // The current version is 1.1, so it's 1.0.
    QVERIFY(qmltypesData.contains("exports: [\"QmlTypeRegistrarTest/WithMethod 1.0\"]"));
}

void tst_qmltyperegistrar::constructibleValueType()
{
    QVERIFY(qmltypesData.contains(
    R"(Component {
        file: "tst_qmltyperegistrar.h"
        name: "Constructible"
        accessSemantics: "value"
        exports: ["QmlTypeRegistrarTest/constructible 1.0"]
        exportMetaObjectRevisions: [256]
        Method {
            name: "Constructible"
            isConstructor: true
            Parameter { name: "i"; type: "int" }
        }
        Method { name: "Constructible"; isCloned: true; isConstructor: true }
    })"));
}

void tst_qmltyperegistrar::anonymousAndUncreatable()
{
    QVERIFY(qmltypesData.contains(
    R"(Component {
        file: "tst_qmltyperegistrar.h"
        name: "AnonymousAndUncreatable"
        accessSemantics: "reference"
        prototype: "QObject"
    })"));
}

void tst_qmltyperegistrar::omitInvisible()
{
    // If it cannot resolve the type a QML_FOREIGN refers to, it should not generate anything.
    QVERIFY(qmltypesData.contains(
                R"(Component { file: "tst_qmltyperegistrar.h"; name: "Invisible"; accessSemantics: "none" })"));
}

void tst_qmltyperegistrar::typedEnum()
{
    QVERIFY(qmltypesData.contains(
    R"(Component {
        file: "tst_qmltyperegistrar.h"
        name: "TypedEnum"
        accessSemantics: "reference"
        prototype: "QObject"
        exports: ["QmlTypeRegistrarTest/TypedEnum 1.0"]
        exportMetaObjectRevisions: [256]
        Enum {
            name: "S"
            type: "short"
            values: ["A", "B", "C"]
        }
        Enum {
            name: "T"
            type: "ushort"
            values: ["D", "E", "F"]
        }
        Enum {
            name: "U"
            type: "qint8"
            values: ["G", "H", "I"]
        }
        Enum {
            name: "V"
            type: "quint8"
            values: ["J", "K", "L"]
        }
    })"));
}

void tst_qmltyperegistrar::listSignal()
{
    QVERIFY(qmltypesData.contains(
    R"(Component {
        file: "tst_qmltyperegistrar.h"
        name: "ListSignal"
        accessSemantics: "reference"
        prototype: "QObject"
        Signal {
            name: "objectListHappened"
            Parameter { type: "QObjectList" }
        }
    })"));
}

void tst_qmltyperegistrar::withNamespace()
{
    QVERIFY(qmltypesData.contains(R"(Component {
        file: "tst_qmltyperegistrar.h"
        name: "Bar"
        accessSemantics: "reference"
        prototype: "QObject"
        Property {
            name: "outerBarProp"
            type: "int"
            read: "bar"
            index: 0
            isReadonly: true
            isConstant: true
        }
    })"));

    QVERIFY(qmltypesData.contains(R"(Component {
        file: "tst_qmltyperegistrar.h"
        name: "Testing::Bar"
        accessSemantics: "reference"
        prototype: "Testing::Foo"
        exports: ["QmlTypeRegistrarTest/Bar 1.0"]
        exportMetaObjectRevisions: [256]
        Property { name: "barProp"; type: "int"; read: "bar"; index: 0; isReadonly: true; isConstant: true }
    })"));

    QVERIFY(qmltypesData.contains(R"(Component {
        file: "tst_qmltyperegistrar.h"
        name: "Testing::Foo"
        accessSemantics: "reference"
        prototype: "QObject"
        Property { name: "fooProp"; type: "int"; read: "foo"; index: 0; isReadonly: true; isConstant: true }
    })"));

    QVERIFY(qmltypesData.contains(R"(Component {
        file: "tst_qmltyperegistrar.h"
        name: "Testing::Inner::Baz"
        accessSemantics: "reference"
        prototype: "Testing::Bar"
        extension: "Bar"
        exports: ["QmlTypeRegistrarTest/Baz 1.0"]
        exportMetaObjectRevisions: [256]
        attachedType: "Testing::Foo"
    })"));
}

void tst_qmltyperegistrar::sequenceRegistration()
{
    QVERIFY(qmltypesData.contains(R"(Component {
        file: "tst_qmltyperegistrar.h"
        name: "std::vector<QByteArray>"
        accessSemantics: "sequence"
        valueType: "QByteArray"
    })"));
}

void tst_qmltyperegistrar::valueTypeSelfReference()
{
    QVERIFY(qmltypesData.contains(R"(Component {
        file: "tst_qmltyperegistrar.h"
        name: "QPersistentModelIndex"
        accessSemantics: "value"
        extension: "QPersistentModelIndexValueType"
    })"));
    QVERIFY(qmltypesData.contains(R"(Component {
        file: "tst_qmltyperegistrar.h"
        name: "QPersistentModelIndexValueType"
        accessSemantics: "value"
        Property { name: "row"; type: "int"; read: "row"; index: 0; isReadonly: true; isFinal: true }
    })"));
}

void tst_qmltyperegistrar::foreignNamespaceFromGadget()
{
    QQmlEngine engine;
    {
        QQmlComponent c(&engine);
        c.setData(QStringLiteral(R"(
            import QtQml
            import QmlTypeRegistrarTest
            QtObject {
                objectName: 'b' + NetworkManager.B
            }
        )").toUtf8(), QUrl("foreignNamespaceFromGadget.qml"));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QCOMPARE(o->objectName(), QStringLiteral("b1"));
    }

    {
        QQmlComponent c(&engine);
        c.setData(QStringLiteral(R"(
            import QtQml
            import QmlTypeRegistrarTest
            QtObject {
                objectName: 'b' + NotNamespaceForeign.B
            }
        )").toUtf8(), QUrl("foreignNamespaceFromGadget2.qml"));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QCOMPARE(o->objectName(), QStringLiteral("b1"));
    }
}

QTEST_MAIN(tst_qmltyperegistrar)
