/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
    QString name; // C++ function name
    QList<QmltcVariable> parameterList; // C++ function parameter list
    QStringList body; // C++ function code
    QQmlJSMetaMethod::Access access = QQmlJSMetaMethod::Public; // access specifier
    QStringList declarationPrefixes;
};

// Represents QML -> C++ compiled function
struct QmltcMethod : QmltcMethodBase
{
    QString returnType; // C++ return type
    QQmlJSMetaMethod::Type type = QQmlJSMetaMethod::Method; // Qt function type
};

// Represents C++ ctor of a type
struct QmltcCtor : QmltcMethodBase
{
    QStringList initializerList; // C++ ctor's initializer list
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
    QmltcCtor basicCtor = {}; // does basic contruction
    QmltcCtor fullCtor = {}; // calls basicCtor, calls init
    QmltcMethod init = {}; // starts object initialization (context setup), calls finalize
    QmltcMethod finalize = {}; // finalizes object (bindings, special interface calls, etc.)

    // member functions: methods, signals and slots
    QList<QmltcMethod> functions;
    // member variables
    QList<QmltcVariable> variables;
    QList<QmltcProperty> properties;

    // QML document root specific:
    std::optional<QmltcVariable> typeCount; // the number of QML types defined in a document
};

// Represents whole QML program, compiled to C++
struct QmltcProgram
{
    QString url; // QML file url
    QString cppPath; // C++ output .cpp path
    QString hPath; // C++ output .h path
    QString outNamespace;
    QSet<QString> includes; // non-default C++ include files
    QmltcMethod urlMethod; // returns QUrl of the QML document

    QList<QmltcType> compiledTypes; // all QML types that are compiled to C++
};

QT_END_NAMESPACE

#endif // QMLTCOUTPUTIR_H
