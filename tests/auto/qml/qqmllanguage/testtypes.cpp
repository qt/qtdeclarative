// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "testtypes.h"

#include <private/qv4qmlcontext_p.h>

static QObject *myTypeObjectSingleton(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);

    return new MyTypeObject();
}

static QJSValue myQJSValueQObjectSingleton(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine);

    QJSValue value = scriptEngine->newQObject(new MyTypeObject());
    return value;
}

void registerTypes()
{
    qmlRegisterInterface<MyInterface>("MyInterface", 1);
    qmlRegisterType<MyQmlObject>("Test",1,0,"MyQmlObject");
    qmlRegisterType<MyQmlObjectWithAttachedCounter>("Test", 1, 0, "MyQmlObjectWithAttachedCounter");
    qmlRegisterType<MyTypeObject>("Test",1,0,"MyTypeObject");
    qmlRegisterType<MyContainer>("Test",1,0,"MyContainer");
    qmlRegisterType<MyPropertyValueSource>("Test",1,0,"MyPropertyValueSource");
    qmlRegisterType<MyDotPropertyObject>("Test",1,0,"MyDotPropertyObject");
    qmlRegisterType<MyNamespace::MyNamespacedType>("Test",1,0,"MyNamespacedType");
    qmlRegisterType<MyNamespace::MySecondNamespacedType>("Test",1,0,"MySecondNamespacedType");
    qmlRegisterUncreatableMetaObject(MyNamespace::staticMetaObject, "Test", 1, 0, "MyNamespace", "Access to enums & flags only");
    qmlRegisterType<MyParserStatus>("Test",1,0,"MyParserStatus");
    qmlRegisterAnonymousType<MyGroupedObject>("Test", 1);
    qmlRegisterType<MyRevisionedClass>("Test",1,0,"MyRevisionedClass");
    qmlRegisterType<MyRevisionedClass,1>("Test",1,1,"MyRevisionedClass");
    qmlRegisterType<MyRevisionedIllegalOverload>("Test",1,0,"MyRevisionedIllegalOverload");
    qmlRegisterType<MyRevisionedLegalOverload>("Test",1,0,"MyRevisionedLegalOverload");
    qmlRegisterType<SomethingKnown>("Test",1,0,"SomethingKnown");

    // Register the uncreatable base class
    qmlRegisterRevision<MyRevisionedBaseClassRegistered,1>("Test",1,1);
    // MyRevisionedSubclass 1.0 uses MyRevisionedClass revision 0
    qmlRegisterType<MyRevisionedSubclass>("Test",1,0,"MyRevisionedSubclass");
    // MyRevisionedSubclass 1.1 uses MyRevisionedClass revision 1
    qmlRegisterType<MyRevisionedSubclass,1>("Test",1,1,"MyRevisionedSubclass");

    // Only version 1.0, but its super class is registered in version 1.1 also
    qmlRegisterType<MySubclass>("Test",1,0,"MySubclass");

    qmlRegisterCustomType<MyCustomParserType>("Test", 1, 0, "MyCustomParserType", new MyCustomParserTypeParser);
    qmlRegisterCustomType<MyCustomParserType>("Test", 1, 0, "MyCustomParserWithEnumType", new EnumSupportingCustomParser);

    qmlRegisterTypeNotAvailable("Test",1,0,"UnavailableType", "UnavailableType is unavailable for testing");

    qmlRegisterType<MyQmlObject>("Test.Version",1,0,"MyQmlObject");
    qmlRegisterType<MyTypeObject>("Test.Version",1,0,"MyTypeObject");
    qmlRegisterType<MyTypeObject>("Test.Version",2,0,"MyTypeObject");

    qmlRegisterType<MyVersion2Class>("Test.VersionOrder", 2,0, "MyQmlObject");
    qmlRegisterType<MyQmlObject>("Test.VersionOrder", 1,0, "MyQmlObject");

    qmlRegisterType<MyEnum1Class>("Test",1,0,"MyEnum1Class");
    qmlRegisterType<MyEnum2Class>("Test",1,0,"MyEnum2Class");
    qmlRegisterType<MyEnumDerivedClass>("Test",1,0,"MyEnumDerivedClass");

    qmlRegisterType<MyReceiversTestObject>("Test",1,0,"MyReceiversTestObject");

    qmlRegisterUncreatableType<MyUncreateableBaseClass>("Test", 1, 0, "MyUncreateableBaseClass", "Cannot create MyUncreateableBaseClass");
    qmlRegisterType<MyCreateableDerivedClass>("Test", 1, 0, "MyCreateableDerivedClass");

    qmlRegisterUncreatableType<MyUncreateableBaseClass,1>("Test", 1, 1, "MyUncreateableBaseClass", "Cannot create MyUncreateableBaseClass");
    qmlRegisterType<MyCreateableDerivedClass,1>("Test", 1, 1, "MyCreateableDerivedClass");

    qmlRegisterExtendedUncreatableType<MyExtendedUncreateableBaseClass, MyExtendedUncreateableBaseClassExtension>("Test", 1, 0, "MyExtendedUncreateableBaseClass", "Cannot create MyExtendedUncreateableBaseClass");
    qmlRegisterExtendedUncreatableType<MyExtendedUncreateableBaseClass, MyExtendedUncreateableBaseClassExtension, 1>("Test", 1, 1, "MyExtendedUncreateableBaseClass", "Cannot create MyExtendedUncreateableBaseClass");
    qmlRegisterType<MyExtendedCreateableDerivedClass>("Test", 1, 0, "MyExtendedCreateableDerivedClass");

    qmlRegisterCustomType<CustomBinding>("Test", 1, 0, "CustomBinding", new CustomBindingParser);
    qmlRegisterCustomType<SimpleObjectWithCustomParser>("Test", 1, 0, "SimpleObjectWithCustomParser", new SimpleObjectCustomParser);

    qmlRegisterCustomExtendedType<SimpleObjectWithCustomParser, SimpleObjectExtension>("Test", 1, 0, "SimpleExtendedObjectWithCustomParser", new SimpleObjectCustomParser);

    qmlRegisterType<RootObjectInCreationTester>("Test", 1, 0, "RootObjectInCreationTester");

    qmlRegisterType<MyCompositeBaseType>("Test", 1, 0, "MyCompositeBaseType");

    qmlRegisterSingletonType<MyTypeObjectSingleton>("Test", 1, 0, "MyTypeObjectSingleton", myTypeObjectSingleton);
    qmlRegisterSingletonType("Test", 1, 0, "MyQJSValueQObjectSingleton", myQJSValueQObjectSingleton);

    qmlRegisterType<MyArrayBufferTestClass>("Test", 1, 0, "MyArrayBufferTestClass");

    qmlRegisterType<LazyDeferredSubObject>("Test", 1, 0, "LazyDeferredSubObject");
    qmlRegisterType<DeferredProperties>("Test", 1, 0, "DeferredProperties");
    qmlRegisterType<ImmediateProperties>("Test", 1, 0, "ImmediateProperties");

    qmlRegisterTypesAndRevisions<Extended, Foreign, ForeignExtended>("Test", 1);
    qmlRegisterTypesAndRevisions<BareSingleton>("Test", 1);
    qmlRegisterTypesAndRevisions<UncreatableSingleton>("Test", 1);

    // Metatype/namespace variation one: Register namespace first

    // The holder type
    qmlRegisterTypesAndRevisions<ObjectTypeHoldingValueTypeForeign1>("Test", 1);

    {
        // A metatype for the namespace to hold the enums
        Q_CONSTINIT static auto metaType = QQmlPrivate::metaTypeForNamespace(
                    [](const QtPrivate::QMetaTypeInterface *) {
            return &ValueTypeWithEnum1::staticMetaObject;
        }, "ValueTypeWithEnum1");
        QMetaType(&metaType).id();
    }

    // The namespace to hold the enums
    qmlRegisterNamespaceAndRevisions(&ValueTypeWithEnum1::staticMetaObject, "Test", 1, nullptr,
                                     &ValueTypeWithEnumForeignNamespace1::staticMetaObject);

    // The value type
    qmlRegisterTypesAndRevisions<ValueTypeWithEnumForeign1>("Test", 1);


    // Metatype/namespace variation two: Register namespace last

    // The holder type
    qmlRegisterTypesAndRevisions<ObjectTypeHoldingValueTypeForeign2>("Test", 1);

    // The value type
    qmlRegisterTypesAndRevisions<ValueTypeWithEnumForeign2>("Test", 1);

    {
        // A metatype for the namespace to hold the enums
        Q_CONSTINIT static auto metaType = QQmlPrivate::metaTypeForNamespace(
                    [](const QtPrivate::QMetaTypeInterface *) {
            return &ValueTypeWithEnum2::staticMetaObject;
        }, "ValueTypeWithEnum2");
        QMetaType(&metaType).id();
    }

    // The namespace to hold the enums
    qmlRegisterNamespaceAndRevisions(&ValueTypeWithEnum2::staticMetaObject, "Test", 1, nullptr,
                                     &ValueTypeWithEnumForeignNamespace2::staticMetaObject);

    qmlRegisterTypesAndRevisions<Large>("Test", 1);
    qmlRegisterTypesAndRevisions<Foo>("Test", 1);

    qmlRegisterTypesAndRevisions<BaseValueType>("ValueTypes", 1);
    qmlRegisterTypesAndRevisions<DerivedValueType>("ValueTypes", 1);
    qmlRegisterTypesAndRevisions<GetterObject>("Test", 1);

    qmlRegisterNamespaceAndRevisions(&TypedEnums::staticMetaObject, "TypedEnums", 1);
    qmlRegisterTypesAndRevisions<ObjectWithEnums>("TypedEnums", 1);
    qmlRegisterTypesAndRevisions<GadgetWithEnums>("TypedEnums", 1);

    QMetaType::registerConverter<UnregisteredValueDerivedType, UnregisteredValueBaseType>();
    qmlRegisterTypesAndRevisions<UnregisteredValueTypeHandler>("Test", 1);

    qmlRegisterTypesAndRevisions<Greeter>("QmlOtherThis", 1);
    qmlRegisterTypesAndRevisions<BirthdayParty>("People", 1);
    qmlRegisterTypesAndRevisions<AttachedInCtor>("Test", 1);

    qmlRegisterTypesAndRevisions<ByteArrayReceiver>("Test", 1);

    qmlRegisterTypesAndRevisions<Counter>("Test", 1);
}

QVariant myCustomVariantTypeConverter(const QString &data)
{
    MyCustomVariantType rv;
    rv.a = data.toInt();
    return QVariant::fromValue(rv);
}


void CustomBindingParser::applyBindings(QObject *object, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &bindings)
{
    CustomBinding *customBinding = qobject_cast<CustomBinding*>(object);
    Q_ASSERT(customBinding);
    customBinding->compilationUnit = compilationUnit;
    customBinding->bindings = bindings;
}

void CustomBinding::componentComplete()
{
    Q_ASSERT(m_target);

    foreach (const QV4::CompiledData::Binding *binding, bindings) {
        QString name = compilationUnit->stringAt(binding->propertyNameIndex);

        int bindingId = binding->value.compiledScriptIndex;

        QQmlRefPointer<QQmlContextData> context = QQmlContextData::get(qmlContext(this));

        QQmlProperty property(m_target, name, qmlContext(this));
        QV4::Scope scope(qmlEngine(this)->handle());
        QV4::Scoped<QV4::QmlContext> qmlContext(scope, QV4::QmlContext::create(scope.engine->rootContext(), context, m_target));
        QQmlBinding *qmlBinding = QQmlBinding::create(&QQmlPropertyPrivate::get(property)->core,
                                                      compilationUnit->runtimeFunctions[bindingId], m_target, context, qmlContext);
        qmlBinding->setTarget(property);
        QQmlPropertyPrivate::setBinding(property, qmlBinding);
    }
}

void EnumSupportingCustomParser::verifyBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &bindings)
{
    if (bindings.size() != 1) {
        error(bindings.first(), QStringLiteral("Custom parser invoked incorrectly for unit test"));
        return;
    }

    const QV4::CompiledData::Binding *binding = bindings.first();
    if (compilationUnit->stringAt(binding->propertyNameIndex) != QStringLiteral("foo")) {
        error(binding, QStringLiteral("Custom parser invoked with the wrong property name"));
        return;
    }

    if (binding->type() != QV4::CompiledData::Binding::Type_Script) {
        error(binding, QStringLiteral("Custom parser invoked with the wrong property value. Expected script that evaluates to enum"));
        return;
    }
    QByteArray script = compilationUnit->stringAt(binding->stringIndex).toUtf8();
    bool ok;
    int v = evaluateEnum(script, &ok);
    if (!ok) {
        error(binding, QStringLiteral("Custom parser invoked with the wrong property value. Script did not evaluate to enum"));
        return;
    }
    if (v != MyEnum1Class::A_13) {
        error(binding, QStringLiteral("Custom parser invoked with the wrong property value. Enum value is not the expected value."));
        return;
    }
}

void SimpleObjectCustomParser::applyBindings(QObject *object, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &, const QList<const QV4::CompiledData::Binding *> &bindings)
{
    SimpleObjectWithCustomParser *o = qobject_cast<SimpleObjectWithCustomParser*>(object);
    Q_ASSERT(o);
    o->setCustomBindingsCount(bindings.size());
}


MyQmlObject::MyQmlObject()
    : m_value(-1)
    , m_interface(nullptr)
    , m_qmlobject(nullptr)
    , m_childAddedEventCount(0)
{
    qRegisterMetaType<MyCustomVariantType>("MyCustomVariantType");
}

bool MyQmlObject::event(QEvent *event)
{
    if (event->type() == QEvent::ChildAdded)
        m_childAddedEventCount++;
    return QObject::event(event);
}

int MyQmlObjectWithAttachedCounter::attachedCount = 0;

UncreatableSingleton *UncreatableSingleton::instance()
{
    static UncreatableSingleton instance;
    return &instance;
}
