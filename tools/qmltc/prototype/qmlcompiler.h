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

#ifndef QMLCOMPILER_H
#define QMLCOMPILER_H

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
#include <QtCore/qstringlist.h>

#include <private/qqmljscompiler_p.h>
#include <private/qqmljsmetatypes_p.h>

struct Options
{
    QString outputCppFile;
    QString outputHFile;
    QString moduleUri;
    QString resourcePath;
    QString outNamespace;
    bool debugGenerateLineDirective = false;
};

// TODO: rename the classes into Qmltc* pattern

// Below are the classes that represent a compiled QML types in a string data
// form. These classes should be used to generate C++ code.

// Represents QML->C++ compiled enumeration type
struct QQmlJSAotEnum
{
    QString cppType; // C++ type of enum
    QStringList keys; // enumerator
    QStringList values; // enumerator value
    QString ownMocLine; // special MOC line that follows enum declaration

    QQmlJSAotEnum() = default;
    QQmlJSAotEnum(const QString &t, const QStringList &ks, const QStringList &vs, const QString &l)
        : cppType(t), keys(ks), values(vs), ownMocLine(l)
    {
    }
};

// Represents C++ member variable
struct QQmlJSAotVariable
{
    QString cppType; // C++ type of a variable
    QString name; // variable name
    QString defaultValue; // optional default value

    QQmlJSAotVariable() = default;
    QQmlJSAotVariable(const QString &t, const QString &n, const QString &v)
        : cppType(t), name(n), defaultValue(v)
    {
    }
};

struct QQmlJSAotProperty : QQmlJSAotVariable
{
    QString containingClass;
    QString signalName;

    QQmlJSAotProperty() = default;
    QQmlJSAotProperty(const QString t, const QString &n, const QString &c, const QString &s)
        : QQmlJSAotVariable(t, n, QString()), containingClass(c), signalName(s)
    {
    }
};

struct QQmlJSAotMethodBase
{
    QString returnType; // C++ return type
    QString name; // C++ function name
    QList<QQmlJSAotVariable> parameterList; // C++ function parameter list
    QStringList body; // C++ code of function body by line
    QStringList declPreambles; // e.g. "static" keyword
    QStringList modifiers; // e.g. cv-qualifiers, ref-qualifier, noexcept, attributes

    // TODO: these are only needed for Component.onCompleted -- any better way?
    QStringList firstLines; // C++ to run at the very beginning of a function
    QStringList lastLines; // C++ to run at the very end of a function

    QQmlJSMetaMethod::Access access = QQmlJSMetaMethod::Public; // access specifier
};

// Represents QML->C++ compiled member function
struct QQmlJSAotMethod : QQmlJSAotMethodBase
{
    QQmlJSMetaMethod::Type type = QQmlJSMetaMethod::Method; // Qt function type
    bool userVisible = false; // tells if a function is prioritized during the output generation
};

// Represents C++ special member function
struct QQmlJSAotSpecialMethod : QQmlJSAotMethodBase
{
    QStringList initializerList; // C++ ctor initializer list
};

// Represents QML->C++ compiled class type that is used for C++ code generation
struct QQmlJSAotObject
{
    QString cppType; // C++ class name of the QML object
    QStringList baseClasses; // C++ class names of base classes
    // TODO: also add "creation string"?
    QStringList mocCode;
    QStringList otherCode; // code that doesn't fit any category, e.g. friend declarations

    // TODO: does it really need to be QHash and not QList?

    // member types: enumerations and child types
    QList<QQmlJSAotEnum> enums;
    QList<QQmlJSAotObject> children; // these are pretty much always empty
    // special member functions
    QQmlJSAotSpecialMethod baselineCtor = {}; // does primary initialization
    QQmlJSAotMethod init = {}; // begins secondary initialization
    QQmlJSAotMethod endInit = {}; // ends initialization (with binding setup)
    QQmlJSAotMethod completeComponent = {}; // calls componentComplete()
    QQmlJSAotMethod finalizeComponent = {}; // invokes finalizer callbacks
    QQmlJSAotMethod handleOnCompleted = {}; // calls Component.onCompleted
    QQmlJSAotSpecialMethod externalCtor = {}; // calls baselineCtor, calls init
    std::optional<QQmlJSAotSpecialMethod> dtor = {};
    // member functions: methods, signals and slots
    QList<QQmlJSAotMethod> functions;
    // member variables
    QList<QQmlJSAotVariable> variables;
    // member properties
    QList<QQmlJSAotProperty> properties;

    // TODO: only needed for binding callables - should be revisited
    bool ignoreInit = false; // specifies whether init and externalCtor should be ignored
};

struct QQmlJSProgram
{
    QList<QQmlJSAotObject> compiledObjects;
    QQmlJSAotMethod urlMethod;
    QString url;
    QString hPath;
    QString cppPath;
    QString outNamespace;
    QSet<QString> includes;
};

#endif // QMLCOMPILER_H
