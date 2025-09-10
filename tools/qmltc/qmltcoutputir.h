// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTCOUTPUTIR_H
#define QMLTCOUTPUTIR_H

#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qset.h>

#include <private/qqmljsmetatypes_p.h>

#include <optional>

QT_BEGIN_NAMESPACE

// Below are the classes that represent compiled QML types in a string data
// form. These classes are used to generate C++ code.

// Represents C++ variable
struct QmltcVariable
{
    QString cppType; // C++ type of a variable
    QString name; // variable name
    QString defaultValue; // optional initialization value

    QmltcVariable() = default;
    // special ctor for QList's emplace back
    QmltcVariable(const QString &t, const QString &n, const QString &v = QString())
        : cppType(t), name(n), defaultValue(v)
    {
    }
};

struct QmltcProperty : QmltcVariable
{
    QString containingClass;
    QString signalName;

    QmltcProperty() = default;
    QmltcProperty(const QString t, const QString &n, const QString &c, const QString &s)
        : QmltcVariable(t, n), containingClass(c), signalName(s)
    {
    }
};

// Represents QML -> C++ compiled enumeration type
struct QmltcEnum
{
    QString cppType; // C++ type of an enum
    QStringList keys; // enumerator keys
    QStringList values; // enumerator values
    QString ownMocLine; // special MOC line that follows enum declaration

    QmltcEnum() = default;
    QmltcEnum(const QString &t, const QStringList &ks, const QStringList &vs, const QString &l)
        : cppType(t), keys(ks), values(vs), ownMocLine(l)
    {
    }
};

struct QmltcMethodBase
{
    QStringList comments; // C++ comments
    QString name; // C++ function name
    QList<QmltcVariable> parameterList; // C++ function parameter list
    QStringList body; // C++ function code
    QQmlJSMetaMethod::Access access = QQmlJSMetaMethod::Public; // access specifier
    QStringList declarationPrefixes;
    QStringList modifiers; // cv-qualifiers, ref-qualifier, noexcept, attributes
};

// Represents QML -> C++ compiled function
struct QmltcMethod : QmltcMethodBase
{
    QString returnType; // C++ return type
    QQmlJSMetaMethodType type = QQmlJSMetaMethodType::Method; // Qt function type

    // TODO: should be a better way to handle this
    bool userVisible = false; // tells if a function is prioritized during the output generation
};

// Represents C++ ctor of a type
struct QmltcCtor : QmltcMethodBase
{
    QStringList initializerList; // C++ ctor's initializer list
};

// Represents C++ dtor of a type
struct QmltcDtor : QmltcMethodBase
{
};

// Represents a generated class that knows how to set the public,
// writable properties of a compiled QML -> C++ type.
// This is generally intended to be available for the root of the
// document to allow the user to set the initial values for
// properties, when creating a component, with support for strong
// typing.
struct QmltcPropertyInitializer {
    QString name;

    QmltcCtor constructor;

    // A member containing a reference to the object for which the
    // properties should be set.
    QmltcVariable component;

    // A member containing a cache of properties that were actually
    // set that can be referenced later..
    QmltcVariable initializedCache;

    // Setter methods for each property.
    QList<QmltcMethod> propertySetters;
};

// Represents a generated class that contains a bundle of values to
// initialize the required properties of a type.
//
// This is generally intended to be available for the root component
// of the document, where it will be used as a constructor argument to
// force the user to provide initial values for the required
// properties of the constructed type.
struct QmltcRequiredPropertiesBundle {
    QString name;

    QList<QmltcVariable> members;
};

// Represents QML -> C++ compiled type
struct QmltcType
{
    QString cppType; // C++ type of the QML type
    QStringList baseClasses; // C++ type names of base classes
    QStringList mocCode; // Qt MOC code
    QStringList otherCode; // Random code that doesn't fit any category, e.g. friend declarations

    // member types: enumerations and child types
    QList<QmltcEnum> enums;
    QList<QmltcType> children; // these are pretty much always empty

    // special member functions:
    QmltcCtor baselineCtor {}; // does basic contruction
    QmltcCtor externalCtor {}; // calls basicCtor, calls init
    QmltcMethod init {}; // starts object initialization (context setup), calls finalize
    QmltcMethod beginClass {}; // calls QQmlParserStatus::classBegin()
    QmltcMethod endInit {}; // ends object initialization (with "simple" bindings setup)
    QmltcMethod setComplexBindings {}; // sets up "complex" (e.g. script) bindings
    QmltcMethod completeComponent {}; // calls QQmlParserStatus::componentComplete()
    QmltcMethod finalizeComponent {}; // calls QQmlFinalizerHook::componentFinalized()
    QmltcMethod handleOnCompleted {}; // calls Component.onCompleted

    std::optional<QmltcDtor> dtor {};

    // member functions: methods, signals and slots
    QList<QmltcMethod> functions;
    // member variables
    QList<QmltcVariable> variables;
    QList<QmltcProperty> properties;

    // QML document root specific:
    std::optional<QmltcMethod> typeCount; // the number of QML types defined in a document

    // TODO: only needed for binding callables - should not be needed, generally
    bool ignoreInit = false; // specifies whether init and externalCtor should be ignored

    // needed for singletons
    std::optional<QmltcMethod> staticCreate{};

    // A proxy class that provides a restricted interface that only
    // allows setting the properties of the type.
    QmltcPropertyInitializer propertyInitializer{};

    std::optional<QmltcRequiredPropertiesBundle> requiredPropertiesBundle{};
};

// Represents whole QML program, compiled to C++
struct QmltcProgram
{
    QString url; // QML file url
    QString cppPath; // C++ output .cpp path
    QString hPath; // C++ output .h path
    QString outNamespace;
    QString exportMacro; // if not empty, the macro that should be used to export the generated
                         // classes
    QSet<QString> includes; // non-default C++ include files
    QmltcMethod urlMethod; // returns QUrl of the QML document

    QList<QmltcType> compiledTypes; // all QML types that are compiled to C++
};

QT_END_NAMESPACE

#endif // QMLTCOUTPUTIR_H
