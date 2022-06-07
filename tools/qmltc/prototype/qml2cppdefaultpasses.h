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

QT_BEGIN_NAMESPACE

// checks whether some names in QQmlJSScope types:
// - are known C++ vocabulary items
// - are defined several times (e.g. property or enum with the same name appears
//   twice)
void checkForNamingCollisionsWithCpp(const Qml2CppContext &context,
                                     QList<QQmlJSScope::Ptr> &objects);

// ensures that all QQmlJSScope objects have unique internalName() and checks
// whether some name is C++ reserved keyword
void makeUniqueCppNames(const Qml2CppContext &context, QList<QQmlJSScope::Ptr> &objects);

// sets up QML-originated base types of \a objects and \a objects themselves.
// processed types are expected to be generated to C++. returns a set of QML
// originated base types for all \a objects
void setupQmlCppTypes(const Qml2CppContext &context, QList<QQmlJSScope::Ptr> &objects);

// finds all required C++ include files that are needed for the generated C++
QSet<QString> findCppIncludes(const Qml2CppContext &context, QList<QQmlJSScope::Ptr> &objects);

QT_END_NAMESPACE

#endif // QML2CPPPASSES_H
