// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qstandardpaths.h>
#include <qtest.h>
#include <qqml.h>
#include <qqmlprivate.h>
#include <qqmlengine.h>
#include <qqmlcomponent.h>

#include <private/qqmlmetatype_p.h>
#include <private/qqmlpropertyvalueinterceptor_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlanybinding_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

using namespace Qt::StringLiterals;

class tst_qqmlmetatype : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlmetatype() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;

    void qmlParserStatusCast();
    void qmlPropertyValueSourceCast();
    void qmlPropertyValueInterceptorCast();
    void qmlType();
    void invalidQmlTypeName();
    void prettyTypeName();
    void registrationType();
    void compositeType();
    void externalEnums();

    void interceptorAPI();

    void isList();

    void defaultObject();
    void unregisterCustomType();
    void unregisterCustomSingletonType();

    void normalizeUrls();
    void unregisterAttachedProperties();
    void revisionedGroupedProperties();

    void enumsInRecursiveImport_data();
    void enumsInRecursiveImport();

    void revertValueTypeAnimation();

    void clearPropertyCaches();
    void builtins();
};

class TestType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo)

    Q_CLASSINFO("DefaultProperty", "foo")
public:
    int foo() { return 0; }
};
QML_DECLARE_TYPE(TestType);

class TestType2 : public QObject
{
    Q_OBJECT
};

class TestType3 : public QObject
{
    Q_OBJECT
};

class ExternalEnums : public QObject
{
    Q_OBJECT
    Q_ENUMS(QStandardPaths::StandardLocation QStandardPaths::LocateOptions)
public:
    ExternalEnums(QObject *parent = nullptr) : QObject(parent) {}

    static QObject *create(QQmlEngine *engine, QJSEngine *scriptEngine) {
        Q_UNUSED(scriptEngine);
        return new ExternalEnums(engine);
    }
};
QML_DECLARE_TYPE(ExternalEnums);

QObject *testTypeProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);
    return new TestType();
}

class ParserStatusTestType : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    void classBegin() override {}
    void componentComplete() override {}
    Q_CLASSINFO("DefaultProperty", "foo") // Missing default property
    Q_INTERFACES(QQmlParserStatus)
};
QML_DECLARE_TYPE(ParserStatusTestType);

class ValueSourceTestType : public QObject, public QQmlPropertyValueSource
{
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueSource)
public:
    void setTarget(const QQmlProperty &) override {}
};
QML_DECLARE_TYPE(ValueSourceTestType);

class ValueInterceptorTestType : public QObject, public QQmlPropertyValueInterceptor
{
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueInterceptor)
public:
    void setTarget(const QQmlProperty &) override {}
    void write(const QVariant &) override {}
};
QML_DECLARE_TYPE(ValueInterceptorTestType);

void tst_qqmlmetatype::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<TestType>("Test", 1, 0, "TestType");
    qmlRegisterSingletonType<TestType>("Test", 1, 0, "TestTypeSingleton", testTypeProvider);
    qmlRegisterType<ParserStatusTestType>("Test", 1, 0, "ParserStatusTestType");
    qmlRegisterType<ValueSourceTestType>("Test", 1, 0, "ValueSourceTestType");
    qmlRegisterType<ValueInterceptorTestType>("Test", 1, 0, "ValueInterceptorTestType");

    QUrl testTypeUrl(testFileUrl("CompositeType.qml"));
    qmlRegisterType(testTypeUrl, "Test", 1, 0, "TestTypeComposite");
}

void tst_qqmlmetatype::qmlParserStatusCast()
{
    QVERIFY(!QQmlMetaType::qmlType(QMetaType::fromType<int>()).isValid());
    QVERIFY(QQmlMetaType::qmlType(QMetaType::fromType<TestType *>()).isValid());
    QCOMPARE(QQmlMetaType::qmlType(QMetaType::fromType<TestType *>()).parserStatusCast(), -1);
    QVERIFY(QQmlMetaType::qmlType(QMetaType::fromType<ValueSourceTestType *>()).isValid());
    QCOMPARE(QQmlMetaType::qmlType(QMetaType::fromType<ValueSourceTestType *>()).parserStatusCast(), -1);

    QVERIFY(QQmlMetaType::qmlType(QMetaType::fromType<ParserStatusTestType *>()).isValid());
    int cast = QQmlMetaType::qmlType(QMetaType::fromType<ParserStatusTestType *>()).parserStatusCast();
    QVERIFY(cast != -1);
    QVERIFY(cast != 0);

    ParserStatusTestType t;
    QVERIFY(reinterpret_cast<char *>((QObject *)&t) != reinterpret_cast<char *>((QQmlParserStatus *)&t));

    QQmlParserStatus *status = reinterpret_cast<QQmlParserStatus *>(reinterpret_cast<char *>((QObject *)&t) + cast);
    QCOMPARE(status, (QQmlParserStatus*)&t);
}

void tst_qqmlmetatype::qmlPropertyValueSourceCast()
{
    QVERIFY(!QQmlMetaType::qmlType(QMetaType::fromType<int>()).isValid());
    QVERIFY(QQmlMetaType::qmlType(QMetaType::fromType<TestType *>()).isValid());
    QCOMPARE(QQmlMetaType::qmlType(QMetaType::fromType<TestType *>()).propertyValueSourceCast(), -1);
    QVERIFY(QQmlMetaType::qmlType(QMetaType::fromType<ParserStatusTestType *>()).isValid());
    QCOMPARE(QQmlMetaType::qmlType(QMetaType::fromType<ParserStatusTestType *>()).propertyValueSourceCast(), -1);

    QVERIFY(QQmlMetaType::qmlType(QMetaType::fromType<ValueSourceTestType *>()).isValid());
    int cast = QQmlMetaType::qmlType(QMetaType::fromType<ValueSourceTestType *>()).propertyValueSourceCast();
    QVERIFY(cast != -1);
    QVERIFY(cast != 0);

    ValueSourceTestType t;
    QVERIFY(reinterpret_cast<char *>((QObject *)&t) != reinterpret_cast<char *>((QQmlPropertyValueSource *)&t));

    QQmlPropertyValueSource *source = reinterpret_cast<QQmlPropertyValueSource *>(reinterpret_cast<char *>((QObject *)&t) + cast);
    QCOMPARE(source, (QQmlPropertyValueSource*)&t);
}

void tst_qqmlmetatype::qmlPropertyValueInterceptorCast()
{
    QVERIFY(!QQmlMetaType::qmlType(QMetaType::fromType<int>()).isValid());
    QVERIFY(QQmlMetaType::qmlType(QMetaType::fromType<TestType *>()).isValid());
    QCOMPARE(QQmlMetaType::qmlType(QMetaType::fromType<TestType *>()).propertyValueInterceptorCast(), -1);
    QVERIFY(QQmlMetaType::qmlType(QMetaType::fromType<ParserStatusTestType *>()).isValid());
    QCOMPARE(QQmlMetaType::qmlType(QMetaType::fromType<ParserStatusTestType *>()).propertyValueInterceptorCast(), -1);

    QVERIFY(QQmlMetaType::qmlType(QMetaType::fromType<ValueInterceptorTestType *>()).isValid());
    int cast = QQmlMetaType::qmlType(QMetaType::fromType<ValueInterceptorTestType *>()).propertyValueInterceptorCast();
    QVERIFY(cast != -1);
    QVERIFY(cast != 0);

    ValueInterceptorTestType t;
    QVERIFY(reinterpret_cast<char *>((QObject *)&t) != reinterpret_cast<char *>((QQmlPropertyValueInterceptor *)&t));

    QQmlPropertyValueInterceptor *interceptor = reinterpret_cast<QQmlPropertyValueInterceptor *>(reinterpret_cast<char *>((QObject *)&t) + cast);
    QCOMPARE(interceptor, (QQmlPropertyValueInterceptor*)&t);
}

void tst_qqmlmetatype::qmlType()
{
    QQmlType type = QQmlMetaType::qmlType(QString("ParserStatusTestType"), QString("Test"),
                                          QTypeRevision::fromVersion(1, 0));
    QVERIFY(type.isValid());
    QVERIFY(type.module() == QHashedString("Test"));
    QVERIFY(type.elementName() == QLatin1String("ParserStatusTestType"));
    QCOMPARE(type.qmlTypeName(), QLatin1String("Test/ParserStatusTestType"));

    type = QQmlMetaType::qmlType("Test/ParserStatusTestType", QTypeRevision::fromVersion(1, 0));
    QVERIFY(type.isValid());
    QVERIFY(type.module() == QHashedString("Test"));
    QVERIFY(type.elementName() == QLatin1String("ParserStatusTestType"));
    QCOMPARE(type.qmlTypeName(), QLatin1String("Test/ParserStatusTestType"));
}

void tst_qqmlmetatype::invalidQmlTypeName()
{
    QTest::ignoreMessage(QtWarningMsg, "Invalid QML element name \"testtype\"; type names must begin with an uppercase letter");
    QTest::ignoreMessage(QtWarningMsg, "Invalid QML element name \"Test$Type\"");
    QTest::ignoreMessage(QtWarningMsg, "Invalid QML element name \"EndingInSlash/\"");

    QCOMPARE(qmlRegisterType<TestType>("TestNamespace", 1, 0, "Test$Type"), -1); // should fail due to invalid QML type name.
    QCOMPARE(qmlRegisterType<TestType>("Test", 1, 0, "EndingInSlash/"), -1);
    QCOMPARE(qmlRegisterType<TestType>("Test", 1, 0, "testtype"), -1);
}

void tst_qqmlmetatype::prettyTypeName()
{
    TestType2 obj2;
    QCOMPARE(QQmlMetaType::prettyTypeName(&obj2), QString("TestType2"));
    QVERIFY(qmlRegisterType<TestType2>("Test", 1, 0, "") >= 0);
    QCOMPARE(QQmlMetaType::prettyTypeName(&obj2), QString("TestType2"));

    TestType3 obj3;
    QCOMPARE(QQmlMetaType::prettyTypeName(&obj3), QString("TestType3"));
    QVERIFY(qmlRegisterType<TestType3>("Test", 1, 0, "OtherName") >= 0);
    QCOMPARE(QQmlMetaType::prettyTypeName(&obj3), QString("OtherName"));
}

void tst_qqmlmetatype::isList()
{
    QCOMPARE(QQmlMetaType::isList(QMetaType {}), false);
    QCOMPARE(QQmlMetaType::isList(QMetaType::fromType<int>()), false);
    QCOMPARE(QQmlMetaType::isList(QMetaType::fromType<QQmlListProperty<TestType> >()), true);
}

void tst_qqmlmetatype::defaultObject()
{
    QVERIFY(!QQmlMetaType::defaultProperty(&QObject::staticMetaObject).name());
    QVERIFY(!QQmlMetaType::defaultProperty(&ParserStatusTestType::staticMetaObject).name());
    QCOMPARE(QString(QQmlMetaType::defaultProperty(&TestType::staticMetaObject).name()), QString("foo"));

    QObject o;
    TestType t;
    ParserStatusTestType p;

    QVERIFY(QQmlMetaType::defaultProperty((QObject *)nullptr).name() == nullptr);
    QVERIFY(!QQmlMetaType::defaultProperty(&o).name());
    QVERIFY(!QQmlMetaType::defaultProperty(&p).name());
    QCOMPARE(QString(QQmlMetaType::defaultProperty(&t).name()), QString("foo"));
}

void tst_qqmlmetatype::registrationType()
{
    QQmlType type = QQmlMetaType::qmlType(QString("TestType"), QString("Test"),
                                          QTypeRevision::fromVersion(1, 0));
    QVERIFY(type.isValid());
    QVERIFY(!type.isInterface());
    QVERIFY(!type.isSingleton());
    QVERIFY(!type.isComposite());

    type = QQmlMetaType::qmlType(QString("TestTypeSingleton"), QString("Test"),
                                 QTypeRevision::fromVersion(1, 0));
    QVERIFY(type.isValid());
    QVERIFY(!type.isInterface());
    QVERIFY(type.isSingleton());
    QVERIFY(!type.isComposite());

    type = QQmlMetaType::qmlType(QString("TestTypeComposite"), QString("Test"),
                                 QTypeRevision::fromVersion(1, 0));
    QVERIFY(type.isValid());
    QVERIFY(!type.isInterface());
    QVERIFY(!type.isSingleton());
    QVERIFY(type.isComposite());
}

void tst_qqmlmetatype::compositeType()
{
    QQmlEngine engine;

    //Loading the test file also loads all composite types it imports
    QQmlComponent c(&engine, testFileUrl("testImplicitComposite.qml"));
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);

    QQmlType type = QQmlMetaType::qmlType(QString("ImplicitType"), QString(""),
                                          QTypeRevision::fromVersion(1, 0));
    QVERIFY(type.isValid());
    QVERIFY(type.module().isEmpty());
    QCOMPARE(type.elementName(), QLatin1String("ImplicitType"));
    QCOMPARE(type.qmlTypeName(), QLatin1String("ImplicitType"));
    QCOMPARE(type.sourceUrl(), testFileUrl("ImplicitType.qml"));
}

void tst_qqmlmetatype::externalEnums()
{
    QQmlEngine engine;
    qmlRegisterSingletonType<ExternalEnums>("x.y.z", 1, 0, "ExternalEnums", ExternalEnums::create);

    QQmlComponent c(&engine, testFileUrl("testExternalEnums.qml"));
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(obj);
    QVariant a = obj->property("a");
    QCOMPARE(a.typeId(), QMetaType::Int);
    QCOMPARE(a.toInt(), int(QStandardPaths::DocumentsLocation));
    QVariant b = obj->property("b");
    QCOMPARE(b.typeId(), QMetaType::Int);
    QCOMPARE(b.toInt(), int(QStandardPaths::DocumentsLocation));

}

class ForwardAndLogInterceptor : public QObject, public QQmlPropertyValueInterceptor {
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueInterceptor)
public:

    // QQmlPropertyValueInterceptor interface
    void setTarget(const QQmlProperty &property) override
    {
        m_property = property;
    }
    void write(const QVariant &value) override
    {
        interceptedWrite = true;
        QQmlPropertyPrivate::write(m_property, value, QQmlPropertyData::BypassInterceptor);
    }
    bool bindable(QUntypedBindable *bindable, QUntypedBindable target) override
    {
        interceptedBindable = true;
        *bindable = target;
        return true;
    }

    QQmlProperty m_property;
    bool interceptedBindable = false;
    bool interceptedWrite = false;
};

void tst_qqmlmetatype::interceptorAPI()
{
    qmlRegisterType<ForwardAndLogInterceptor>("test", 1, 0, "Interceptor");
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("interceptorApi.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY2(obj, qPrintable(component.errorString()));

    auto interceptor = obj->property("i").value<ForwardAndLogInterceptor *>();
    QVERIFY(interceptor->interceptedBindable);
    QVERIFY(interceptor->interceptedWrite);

    QQmlProperty objectName(obj.get(), "objectName");
    QProperty<QString> hello(u"Hello, World!"_s);
    QQmlAnyBinding binding;
    binding = Qt::makePropertyBinding(hello);
    interceptor->interceptedBindable = false;
    binding.installOn(objectName, QQmlAnyBinding::RespectInterceptors);
    QVERIFY(interceptor->interceptedBindable);
    binding = QQmlAnyBinding::takeFrom(objectName);
    objectName.write("bar");
    interceptor->interceptedBindable = false;
    binding.installOn(objectName, QQmlAnyBinding::IgnoreInterceptors);
    QVERIFY(!interceptor->interceptedBindable);
    QCOMPARE(objectName.read().toString(), hello.value());
}

class Controller1 : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString string MEMBER m_string)
    Q_PROPERTY(Controller1Enum enumVal MEMBER m_enumVal)
public:
    enum Controller1Enum {
        ENUM_VALUE_1 = 1,
        ENUM_VALUE_2 = 2
    };
    Q_ENUMS(Controller1Enum)

    Controller1(QObject *parent = nullptr) : QObject(parent), m_string("Controller #1"),
        m_enumVal(ENUM_VALUE_1)
    {}
private:
    QString m_string;
    Controller1Enum m_enumVal;
};

class Controller2 : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString string MEMBER m_string)
    Q_PROPERTY(Controller2Enum enumVal MEMBER m_enumVal)
public:
    enum Controller2Enum {
        ENUM_VALUE_1 = 111,
        ENUM_VALUE_2 = 222
    };
    Q_ENUMS(Controller2Enum)

    Controller2(QObject *parent = nullptr) : QObject(parent), m_string("Controller #2"),
        m_enumVal(ENUM_VALUE_1)
    {}
private:
    QString m_string;
    Controller2Enum m_enumVal;
};

void tst_qqmlmetatype::unregisterCustomType()
{
    int controllerId = 0;
    {
        QQmlEngine engine;
        QQmlType type = QQmlMetaType::qmlType(QString("Controller"), QString("mytypes"),
                                              QTypeRevision::fromVersion(1, 0));
        QVERIFY2(!type.isValid(), "Type is not valid yet");
        controllerId = qmlRegisterType<Controller1>("mytypes", 1, 0, "Controller");
        type = QQmlMetaType::qmlType(QString("Controller"), QString("mytypes"),
                                     QTypeRevision::fromVersion(1, 0));
        QVERIFY2(type.isValid(), "Type is valid now");
        QVERIFY2(!type.isInterface(), "Type is not an interface");
        QVERIFY2(!type.isSingleton(), "Type is not a singleton");
        QVERIFY2(!type.isComposite(), "Types is not a composite type");
        QQmlComponent c(&engine, testFileUrl("testUnregisterCustomType.qml"));
        QScopedPointer<QObject> obj(c.create());
        QVERIFY2(obj, "obj is not null");
        QObject *controller = obj->findChild<QObject *>("controller");
        QVERIFY2(qobject_cast<Controller1 *>(controller), "child 'controller' could be found and is a Controller1*");
        QVariant stringVal = controller->property("string");
        QCOMPARE(stringVal.typeId(), QMetaType::QString);
        QCOMPARE(stringVal.toString(), QStringLiteral("Controller #1"));
        QVariant enumVal = controller->property("enumVal");
        QVERIFY2(QMetaType(enumVal.typeId()).flags() & QMetaType::IsEnumeration, "enumVal's type is enumeratoion");
        QCOMPARE(enumVal.toInt(), 1);
    }
    QQmlMetaType::unregisterType(controllerId);
    {
        QQmlEngine engine;
        QQmlType type = QQmlMetaType::qmlType(QString("Controller"), QString("mytypes"),
                                              QTypeRevision::fromVersion(1, 0));
        QVERIFY2(!type.isValid(), "Type is not valid anymore");
        controllerId = qmlRegisterType<Controller2>("mytypes", 1, 0, "Controller");
        type = QQmlMetaType::qmlType(QString("Controller"), QString("mytypes"),
                                     QTypeRevision::fromVersion(1, 0));
        QVERIFY2(type.isValid(), "Type is valid again");
        QVERIFY2(!type.isInterface(), "Type is not an interface");
        QVERIFY2(!type.isSingleton(), "Type is not a singleton");
        QVERIFY2(!type.isComposite(), "Type is not a composite");
        QQmlComponent c(&engine, testFileUrl("testUnregisterCustomType.qml"));
        QScopedPointer<QObject> obj(c.create());
        QVERIFY2(obj, "obj is not null");
        QObject *controller = obj->findChild<QObject *>("controller");
        QVERIFY2(qobject_cast<Controller2 *>(controller), "child 'controller' could be found and is a Controller2*");
        QVariant stringVal = controller->property("string");
        QCOMPARE(stringVal.typeId(), QMetaType::QString);
        QCOMPARE(stringVal.toString(), QStringLiteral("Controller #2"));
        QVariant enumVal = controller->property("enumVal");
        QVERIFY2(QMetaType(enumVal.typeId()).flags() & QMetaType::IsEnumeration, "enumVal's type is enumeratoion");
        QCOMPARE(enumVal.toInt(), 111);
    }
    QQmlMetaType::unregisterType(controllerId);
    {
        QQmlEngine engine;
        QQmlType type = QQmlMetaType::qmlType(QString("Controller"), QString("mytypes"),
                                              QTypeRevision::fromVersion(1, 0));
        QVERIFY2(!type.isValid(), "Type is not valid anymore");
        controllerId = qmlRegisterType<Controller1>("mytypes", 1, 0, "Controller");
        type = QQmlMetaType::qmlType(QString("Controller"), QString("mytypes"),
                                     QTypeRevision::fromVersion(1, 0));
        QVERIFY2(type.isValid(), "Type is valid again");
        QVERIFY2(!type.isInterface(), "Type is not an interface");
        QVERIFY2(!type.isSingleton(), "Type is not a singleton");
        QVERIFY2(!type.isComposite(), "Type is not a composite");
        QQmlComponent c(&engine, testFileUrl("testUnregisterCustomType.qml"));
        QScopedPointer<QObject> obj(c.create());
        QVERIFY2(obj, "obj is not null");
        QObject *controller = obj->findChild<QObject *>("controller");
        QVERIFY2(qobject_cast<Controller1 *>(controller), "child 'controller' could be found and is a Controller1*");
        QVariant stringVal = controller->property("string");
        QCOMPARE(stringVal.typeId(), QMetaType::QString);
        QCOMPARE(stringVal.toString(), QStringLiteral("Controller #1"));
        QVariant enumVal = controller->property("enumVal");
        QVERIFY2(QMetaType(enumVal.typeId()).flags() & QMetaType::IsEnumeration, "enumVal's type is enumeratoion");
        QCOMPARE(enumVal.toInt(), 1);
    }
}

class StaticProvider1 : public QObject
{
    Q_OBJECT
public:
    StaticProvider1(QObject *parent = nullptr) : QObject(parent) {}
    Q_INVOKABLE QString singletonGetString() { return "StaticProvider #1"; }
};

static QObject* createStaticProvider1(QQmlEngine *, QJSEngine *)
{
    return new StaticProvider1;
}

class StaticProvider2 : public QObject
{
    Q_OBJECT
public:
    StaticProvider2(QObject *parent = nullptr) : QObject(parent) {}
    Q_INVOKABLE QString singletonGetString() { return "StaticProvider #2"; }
};

static QObject* createStaticProvider2(QQmlEngine *, QJSEngine *)
{
    return new StaticProvider2;
}

void tst_qqmlmetatype::unregisterCustomSingletonType()
{
    int staticProviderId = 0;
    {
        QQmlEngine engine;
        staticProviderId = qmlRegisterSingletonType<StaticProvider1>("mytypes", 1, 0, "StaticProvider", createStaticProvider1);
        QQmlType type = QQmlMetaType::qmlType(QString("StaticProvider"), QString("mytypes"),
                                              QTypeRevision::fromVersion(1, 0));
        QVERIFY(type.isValid());
        QVERIFY(!type.isInterface());
        QVERIFY(type.isSingleton());
        QVERIFY(!type.isComposite());
        QQmlComponent c(&engine, testFileUrl("testUnregisterCustomSingletonType.qml"));
        QScopedPointer<QObject> obj(c.create());
        QVERIFY(obj.data());
        QVariant stringVal = obj->property("text");
        QCOMPARE(stringVal.typeId(), QMetaType::QString);
        QCOMPARE(stringVal.toString(), QStringLiteral("StaticProvider #1"));
    }
    QQmlMetaType::unregisterType(staticProviderId);
    {
        QQmlEngine engine;
        staticProviderId = qmlRegisterSingletonType<StaticProvider2>("mytypes", 1, 0, "StaticProvider", createStaticProvider2);
        QQmlType type = QQmlMetaType::qmlType(QString("StaticProvider"), QString("mytypes"),
                                              QTypeRevision::fromVersion(1, 0));
        QVERIFY(type.isValid());
        QVERIFY(!type.isInterface());
        QVERIFY(type.isSingleton());
        QVERIFY(!type.isComposite());
        QQmlComponent c(&engine, testFileUrl("testUnregisterCustomSingletonType.qml"));
        QScopedPointer<QObject> obj(c.create());
        QVERIFY(obj.data());
        QVariant stringVal = obj->property("text");
        QCOMPARE(stringVal.typeId(), QMetaType::QString);
        QCOMPARE(stringVal.toString(), QStringLiteral("StaticProvider #2"));
    }
    QQmlMetaType::unregisterType(staticProviderId);
    {
        QQmlEngine engine;
        staticProviderId = qmlRegisterSingletonType<StaticProvider1>("mytypes", 1, 0, "StaticProvider", createStaticProvider1);
        QQmlType type = QQmlMetaType::qmlType(QString("StaticProvider"), QString("mytypes"),
                                              QTypeRevision::fromVersion(1, 0));
        QVERIFY(type.isValid());
        QVERIFY(!type.isInterface());
        QVERIFY(type.isSingleton());
        QVERIFY(!type.isComposite());
        QQmlComponent c(&engine, testFileUrl("testUnregisterCustomSingletonType.qml"));
        QScopedPointer<QObject> obj(c.create());
        QVERIFY(obj.data());
        QVariant stringVal = obj->property("text");
        QCOMPARE(stringVal.typeId(), QMetaType::QString);
        QCOMPARE(stringVal.toString(), QStringLiteral("StaticProvider #1"));
    }
}

void tst_qqmlmetatype::normalizeUrls()
{
    const QUrl url("qrc:///tstqqmlmetatype/data/CompositeType.qml");
    QVERIFY(!QQmlMetaType::qmlType(url).isValid());
    const auto registrationId = qmlRegisterType(url, "Test", 1, 0, "ResourceCompositeType");
    QVERIFY(QQmlMetaType::qmlType(url, /*includeNonFileImports=*/true).isValid());
    QUrl normalizedURL("qrc:/tstqqmlmetatype/data/CompositeType.qml");
    QVERIFY(QQmlMetaType::qmlType(normalizedURL, /*includeNonFileImports=*/true).isValid());
    QQmlMetaType::unregisterType(registrationId);
    QVERIFY(!QQmlMetaType::qmlType(url, /*includeNonFileImports=*/true).isValid());
}

void tst_qqmlmetatype::unregisterAttachedProperties()
{
    qmlClearTypeRegistrations();

    const QUrl dummy("qrc:///doesnotexist.qml");
    {
        QQmlEngine e;
        QQmlComponent c(&e);
        c.setData("import QtQuick 2.2\n Item { }", dummy);

        const QQmlType attachedType = QQmlMetaType::qmlType("QtQuick/KeyNavigation",
                                                            QTypeRevision::fromVersion(2, 2));
        QCOMPARE(attachedType.attachedPropertiesType(QQmlEnginePrivate::get(&e)),
                 attachedType.metaObject());

        QScopedPointer<QObject> obj(c.create());
        QVERIFY(obj);
    }
}

class Grouped : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop READ prop WRITE setProp NOTIFY propChanged REVISION 1)
    Q_PROPERTY(int prop2 READ prop WRITE setProp NOTIFY prop2Changed REVISION 2)
public:
    int prop() const { return m_prop; }
    void setProp(int prop)
    {
        if (prop != m_prop) {
            m_prop = prop;
            emit propChanged(prop);
            emit prop2Changed(prop);
        }
    }

signals:
    Q_REVISION(1) void propChanged(int prop);
    Q_REVISION(2) void prop2Changed(int prop);

private:
    int m_prop = 0;
};

class MyItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Grouped *grouped READ grouped CONSTANT)
public:
    MyItem() : m_grouped(new Grouped) {}
    Grouped *grouped() const { return m_grouped.data(); }

private:
    QScopedPointer<Grouped> m_grouped;
};

class MyRevisioned : public MyItem
{
    Q_OBJECT
    Q_PROPERTY(int revisioned READ revisioned CONSTANT REVISION 1)
public:
    int revisioned() const { return 12; }
};

void tst_qqmlmetatype::revisionedGroupedProperties()
{
    qmlClearTypeRegistrations();
    qmlRegisterType<MyItem>("GroupedTest", 1, 0, "MyItem");
    qmlRegisterType<MyItem, 1>("GroupedTest", 1, 1, "MyItem");
    qmlRegisterType<MyRevisioned>("GroupedTest", 1, 0, "MyRevisioned");
    qmlRegisterType<MyRevisioned, 1>("GroupedTest", 1, 1, "MyRevisioned");
    qmlRegisterUncreatableType<Grouped>("GroupedTest", 1, 0, "Grouped", "Grouped");
    qmlRegisterUncreatableType<Grouped, 1>("GroupedTest", 1, 1, "Grouped", "Grouped");
    qmlRegisterUncreatableType<Grouped, 2>("GroupedTest", 1, 2, "Grouped", "Grouped");

    {
        QQmlEngine engine;
        QQmlComponent valid(&engine, testFileUrl("revisionedGroupedPropertiesValid.qml"));
        QVERIFY(valid.isReady());
        QScopedPointer<QObject> obj(valid.create());
        QVERIFY(!obj.isNull());
    }

    {
        QQmlEngine engine;
        QQmlComponent invalid(&engine, testFileUrl("revisionedGroupedPropertiesInvalid.qml"));
        QVERIFY(invalid.isError());
    }

    {
        QQmlEngine engine;
        QQmlComponent unversioned(
                    &engine, testFileUrl("revisionedGroupedPropertiesUnversioned.qml"));
        QVERIFY2(unversioned.isReady(), qPrintable(unversioned.errorString()));
        QScopedPointer<QObject> obj(unversioned.create());
        QVERIFY(!obj.isNull());
    }
}

void tst_qqmlmetatype::enumsInRecursiveImport_data()
{
    QTest::addColumn<QString>("importPath");
    QTest::addColumn<QUrl>("componentUrl");

    QTest::addRow("data directory") << dataDirectory()
                                    << testFileUrl("enumsInRecursiveImport.qml");

    // The qrc case behaves differently because we failed to detect the recursion in type loading
    // due to varying numbers of slashes after the "qrc:" in the URLs.
    QTest::addRow("resources") << QStringLiteral("qrc:/data")
                               << QUrl("qrc:/data/enumsInRecursiveImport.qml");
}

void tst_qqmlmetatype::enumsInRecursiveImport()
{
    QFETCH(QString, importPath);
    QFETCH(QUrl, componentUrl);

    qmlClearTypeRegistrations();
    QQmlEngine engine;
    engine.addImportPath(importPath);
    QQmlComponent c(&engine, componentUrl);
    QVERIFY(c.isReady());
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(!obj.isNull());
    QTRY_COMPARE(obj->property("color").toString(), QString("green"));
}

void tst_qqmlmetatype::revertValueTypeAnimation()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("animationOnValueType.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QTRY_COMPARE(o->property("letterSpacing").toDouble(), 24.0);
    QCOMPARE(o->property("pointSize").toDouble(), 12.0);
}

void tst_qqmlmetatype::clearPropertyCaches()
{
    qmlClearTypeRegistrations();
    qmlRegisterType<TestType>("ClearPropertyCaches", 1, 0, "A");
    QQmlPropertyCache::ConstPtr oldCache = QQmlMetaType::propertyCache(&TestType::staticMetaObject);
    QVERIFY(oldCache);
    qmlClearTypeRegistrations();
    qmlRegisterType<TestType>("ClearPropertyCaches", 1, 0, "B");
    QQmlPropertyCache::ConstPtr newCache = QQmlMetaType::propertyCache(&TestType::staticMetaObject);
    QVERIFY(oldCache.data() != newCache.data());
}

template<typename T>
void checkBuiltinBaseType()
{
    const QQmlType type = QQmlMetaType::qmlType(QMetaType::fromType<T>());
    QVERIFY(type.isValid());
    QCOMPARE(type.typeId(), QMetaType::fromType<T>());
    QCOMPARE(type.qListTypeId(), QMetaType::fromType<QList<T>>());
}

template<typename T>
void checkBuiltinListType()
{
    const QQmlType listType = QQmlMetaType::qmlListType(QMetaType::fromType<QList<T>>());
    QVERIFY(listType.isValid());
    QVERIFY(listType.isSequentialContainer());
    QCOMPARE(listType.typeId(), QMetaType::fromType<T>());
    QCOMPARE(listType.qListTypeId(), QMetaType::fromType<QList<T>>());
    QCOMPARE(listType.listMetaSequence().valueMetaType(), QMetaType::fromType<T>());
}

template<typename... T>
void checkBuiltinTypes()
{
    (checkBuiltinBaseType<T>(), ...);
    (checkBuiltinListType<T>(), ...);
}

template<typename T>
void checkNamedBuiltin(const QString &name)
{
    QCOMPARE(QQmlMetaType::qmlType("QML/" + name, QTypeRevision::fromVersion(1, 0)),
             QQmlMetaType::qmlType(QMetaType::fromType<T>()));
}

template<typename T>
void checkObjectBuiltin(const QString &name)
{
    const QQmlType objectType = QQmlMetaType::qmlType(QMetaType::fromType<T *>());
    QVERIFY(objectType.isValid());
    QCOMPARE(objectType.typeId(), QMetaType::fromType<T *>());
    QCOMPARE(objectType.qListTypeId(), QMetaType::fromType<QQmlListProperty<T>>());

    const QQmlType listType = QQmlMetaType::qmlListType(QMetaType::fromType<QQmlListProperty<T>>());
    QVERIFY(listType.isValid());
    QCOMPARE(listType.typeId(), QMetaType::fromType<T *>());
    QCOMPARE(listType.qListTypeId(), QMetaType::fromType<QQmlListProperty<T>>());

    checkNamedBuiltin<T *>(name);
}

void tst_qqmlmetatype::builtins()
{
    qmlClearTypeRegistrations();
    QQmlEngine engine; // registers the builtins

    checkBuiltinTypes<
        QVariant, QJSValue, qint8, quint8, short, ushort, int, uint, qlonglong, qulonglong, float,
        double, QChar, QString, bool, QDateTime, QDate, QTime, QUrl, QByteArray>();

    checkNamedBuiltin<QVariant>("var");
    checkNamedBuiltin<QVariant>("variant");
    checkNamedBuiltin<int>("int");
    checkNamedBuiltin<double>("double");
    checkNamedBuiltin<double>("real");
    checkNamedBuiltin<QString>("string");
    checkNamedBuiltin<bool>("bool");
    checkNamedBuiltin<QDateTime>("date");
    checkNamedBuiltin<QUrl>("url");

#if QT_CONFIG(regularexpression)
    checkBuiltinBaseType<QRegularExpression>();
    checkBuiltinListType<QRegularExpression>();
    checkNamedBuiltin<QRegularExpression>("regexp");
#endif

    // Can't retrieve this one by metatype
    const QQmlType voidType = QQmlMetaType::qmlType("QML/void", QTypeRevision::fromVersion(1, 0));
    QVERIFY(voidType.isValid());
    QCOMPARE(voidType.typeId(), QMetaType());
    QCOMPARE(voidType.qListTypeId(), QMetaType());

    // No separate list types
    checkBuiltinBaseType<std::nullptr_t>();
    checkBuiltinBaseType<QVariantMap>();

    checkObjectBuiltin<QObject>("QtObject");
    checkObjectBuiltin<QQmlComponent>("Component");
}

QTEST_MAIN(tst_qqmlmetatype)

#include "tst_qqmlmetatype.moc"
