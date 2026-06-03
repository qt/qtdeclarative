// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLTCOUTPUTIR_P_H
#define QQMLTCOUTPUTIR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qset.h>

#include <private/qqmljsmetatypes_p.h>

#include <optional>

QT_BEGIN_NAMESPACE

namespace QQmltc {

// Below are the classes that represent compiled QML types in a string data
// form. These classes are used to generate C++ code.

// Represents C++ variable
struct Variable
{
    QString cppType; // C++ type of a variable
    QString name; // variable name
    QString defaultValue; // optional initialization value

    Variable() = default;
    // special ctor for QList's emplace back
    Variable(const QString &t, const QString &n, const QString &v = QString())
        : cppType(t), name(n), defaultValue(v)
    {
    }
};

struct Property : Variable
{
    QString containingClass;
    QString signalName;

    Property() = default;
    Property(const QString t, const QString &n, const QString &c, const QString &s)
        : Variable(t, n), containingClass(c), signalName(s)
    {
    }
};

// Represents QML -> C++ compiled enumeration type
struct Enum
{
    QString cppType; // C++ type of an enum
    QStringList keys; // enumerator keys
    QStringList values; // enumerator values
    QString ownMocLine; // special MOC line that follows enum declaration

    Enum() = default;
    Enum(const QString &t, const QStringList &ks, const QStringList &vs, const QString &l)
        : cppType(t), keys(ks), values(vs), ownMocLine(l)
    {
    }
};

struct MethodBase
{
    QStringList comments; // C++ comments
    QString name; // C++ function name
    QList<Variable> parameterList; // C++ function parameter list
    QStringList body; // C++ function code
    QQmlJSMetaMethod::Access access = QQmlJSMetaMethod::Public; // access specifier
    QStringList declarationPrefixes;
    QStringList modifiers; // cv-qualifiers, ref-qualifier, noexcept, attributes
};

// Represents QML -> C++ compiled function
struct Method : MethodBase
{
    QString returnType; // C++ return type
    QQmlJSMetaMethodType type = QQmlJSMetaMethodType::Method; // Qt function type

    // TODO: should be a better way to handle this
    bool userVisible = false; // tells if a function is prioritized during the output generation
};

// Represents C++ ctor of a type
struct Ctor : MethodBase
{
    QStringList initializerList; // C++ ctor's initializer list
};

// Represents C++ dtor of a type
struct Dtor : MethodBase
{
};

// Represents a generated class that knows how to set the public,
// writable properties of a compiled QML -> C++ type.
// This is generally intended to be available for the root of the
// document to allow the user to set the initial values for
// properties, when creating a component, with support for strong
// typing.
struct PropertyInitializer
{
    QString name;

    Ctor constructor;

    // A member containing a reference to the object for which the
    // properties should be set.
    Variable component;

    // A member containing a cache of properties that were actually
    // set that can be referenced later..
    Variable initializedCache;

    // Setter methods for each property.
    QList<Method> propertySetters;
};

// Represents a generated class that contains a bundle of values to
// initialize the required properties of a type.
//
// This is generally intended to be available for the root component
// of the document, where it will be used as a constructor argument to
// force the user to provide initial values for the required
// properties of the constructed type.
struct RequiredPropertiesBundle
{
    QString name;

    QList<Variable> members;
};

// Represents QML -> C++ compiled type
struct Type
{
    QString cppType; // C++ type of the QML type
    QStringList baseClasses; // C++ type names of base classes
    QStringList mocCode; // Qt MOC code
    QStringList otherCode; // Random code that doesn't fit any category, e.g. friend declarations

    // member types: enumerations and child types
    QList<Enum> enums;
    QList<Type> children; // these are pretty much always empty

    // special member functions:
    Ctor baselineCtor {}; // does basic contruction
    Ctor externalCtor {}; // calls basicCtor, calls init
    Method init {}; // starts object initialization (context setup), calls finalize
    Method beginClass {}; // calls QQmlParserStatus::classBegin()
    Method endInit {}; // ends object initialization (with "simple" bindings setup)
    Method setComplexBindings {}; // sets up "complex" (e.g. script) bindings
    Method completeComponent {}; // calls QQmlParserStatus::componentComplete()
    Method finalizeComponent {}; // calls QQmlFinalizerHook::componentFinalized()
    Method handleOnCompleted {}; // calls Component.onCompleted

    std::optional<Dtor> dtor {};

    // member functions: methods, signals and slots
    QList<Method> functions;
    // member variables
    QList<Variable> variables;
    QList<Property> properties;

    // QML document root specific:
    std::optional<Method> typeCount; // the number of QML types defined in a document

    // TODO: only needed for binding callables - should not be needed, generally
    bool ignoreInit = false; // specifies whether init and externalCtor should be ignored

    // needed for singletons
    std::optional<Method> staticCreate{};

    // A proxy class that provides a restricted interface that only
    // allows setting the properties of the type.
    PropertyInitializer propertyInitializer{};

    std::optional<RequiredPropertiesBundle> requiredPropertiesBundle{};
};

// Represents whole QML program, compiled to C++
struct Program
{
    QString url; // QML file url
    QString cppPath; // C++ output .cpp path
    QString hPath; // C++ output .h path
    QString outNamespace;
    QString exportMacro; // if not empty, the macro that should be used to export the generated
                         // classes
    QSet<QString> includes; // non-default C++ include files
    Method urlMethod; // returns QUrl of the QML document

    QList<Type> compiledTypes; // all QML types that are compiled to C++
};

} // namespace QQmltc

QT_END_NAMESPACE

#endif // QQMLTCOUTPUTIR_P_H
