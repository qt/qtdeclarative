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

#ifndef QML2CPPPASSES_H
#define QML2CPPPASSES_H

#include "prototype/qml2cppcontext.h"

// verifies that each object, property (and etc.) has valid
// QQmlJSScope::ConstPtr associated with it
void verifyTypes(const Qml2CppContext &context, QList<Qml2CppObject> &objects);

// checks whether some names in QQmlJSScope types:
// - are known C++ vocabulary items
// - are defined several times (e.g. property or enum with the same name appears
//   twice)
void checkForNamingCollisionsWithCpp(const Qml2CppContext &context, QList<Qml2CppObject> &objects);

// TODO: the below passes are not "default" (cannot be directly added)

// ensures that all QQmlJSScope objects have unique internalName() and checks
// whether some name is C++ reserved keyword
QHash<QString, qsizetype> makeUniqueCppNames(const Qml2CppContext &context,
                                             QList<Qml2CppObject> &objects);

// sets up QML-originated base types of \a objects and \a objects themselves.
// processed types are expected to be generated to C++. returns a set of QML
// originated base types for all \a objects
QSet<QString> setupQmlCppTypes(const Qml2CppContext &context, QList<Qml2CppObject> &objects);

// resolves and finishes the verification of property aliases (checks that a
// READ method is present and a WRITE method is present as well if the type is a
// value type, etc.). returns aliases which point to ids. must be done after
// setupQmlCppTypes() since some (own) aliased properties are not fully set up
// untile then and thus do not have the necessary information
QSet<QQmlJSMetaProperty> deferredResolveValidateAliases(const Qml2CppContext &context,
                                                        QList<Qml2CppObject> &objects);

// finds all required C++ include files that are needed for the generated C++
QSet<QString> findCppIncludes(const Qml2CppContext &context, QList<Qml2CppObject> &objects);

// finds and resolves explicit QQmlComponent types. returns (essentially) a set
// of QmlIR::Object indices that represent types derived from QQmlComponent. the
// return value is a QHash<> to be compatible with
// findAndResolveImplicitComponents()
QHash<int, int> findAndResolveExplicitComponents(const Qml2CppContext &context,
                                                 QList<Qml2CppObject> &objects);

// finds and resolves implicit QQmlComponent types. returns a mapping from
// QmlIR::Object that is being wrapped into a QQmlComponent to an index of that
// implicit wrapper, which is a synthetic QmlIR::Object
QHash<int, int> findAndResolveImplicitComponents(const Qml2CppContext &context,
                                                 QList<Qml2CppObject> &objects);

void setObjectIds(const Qml2CppContext &context, QList<Qml2CppObject> &objects);

// finds an immediate parent of each to-be-compiled type
QHash<QQmlJSScope::ConstPtr, QQmlJSScope::ConstPtr>
findImmediateParents(const Qml2CppContext &context, QList<Qml2CppObject> &objects);

QSet<QQmlJSScope::ConstPtr> collectIgnoredTypes(const Qml2CppContext &context,
                                                QList<Qml2CppObject> &objects);

void setDeferredBindings(const Qml2CppContext &context, QList<Qml2CppObject> &objects);

#endif // QML2CPPPASSES_H
