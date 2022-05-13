// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QML2CPPPASSES_H
#define QML2CPPPASSES_H

#include "prototype/qml2cppcontext.h"

QT_BEGIN_NAMESPACE

// sets up QML-originated base types of \a objects and \a objects themselves.
// processed types are expected to be generated to C++. returns a set of QML
// originated base types for all \a objects
void setupQmlCppTypes(const Qml2CppContext &context, QList<QQmlJSScope::Ptr> &objects);

// finds all required C++ include files that are needed for the generated C++
QSet<QString> findCppIncludes(const Qml2CppContext &context, QList<QQmlJSScope::Ptr> &objects);

QT_END_NAMESPACE

#endif // QML2CPPPASSES_H
