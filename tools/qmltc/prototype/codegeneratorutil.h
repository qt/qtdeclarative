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

#ifndef CODEGENERATORUTIL_H
#define CODEGENERATORUTIL_H

#include "prototype/qmlcompiler.h"

#include <private/qqmljsscope_p.h>
#include <private/qqmljsmetatypes_p.h>

#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

#include <utility>

struct CodeGeneratorUtility
{
    // magic variable, necessary for correct handling of object bindings: since
    // object creation and binding setup are separated across functions, we
    // fetch a subobject by: QObject::children().at(offset + localIndex)
    //                                              ^
    //                                              childrenOffsetVariable
    // this variable would often be equal to 0, but there's no guarantee. and it
    // is required due to non-trivial aliases dependencies: aliases can
    // reference any object in the document by id, which automatically means
    // that all ids have to be set up before we get to finalization (and the
    // only place for it is init)
    static const QQmlJSAotVariable childrenOffsetVariable;

    // represents QV4::ExecutableCompilationUnit
    static const QQmlJSAotVariable compilationUnitVariable;

    // helper functions:
    static QString toResourcePath(const QString &s)
    {
        return u"QUrl(QStringLiteral(\"qrc:" + s + u"\"))";
    }

    static QString metaPropertyName(const QString &propertyVariableName)
    {
        return propertyVariableName + u"_meta";
    }

    // generate functions:
    static QStringList generate_assignToProperty(const QQmlJSScope::ConstPtr &type,
                                                 const QString &propertyName,
                                                 const QQmlJSMetaProperty &p, const QString &value,
                                                 const QString &accessor,
                                                 bool constructQVariant = false);
    static QStringList generate_assignToSpecialAlias(const QQmlJSScope::ConstPtr &type,
                                                     const QString &propertyName,
                                                     const QQmlJSMetaProperty &p,
                                                     const QString &value, const QString &accessor,
                                                     bool constructQVariant = false);
    // TODO: 3 separate versions: bindable QML, bindable C++, non-bindable C++
    static QStringList
    generate_callExecuteRuntimeFunction(const QString &url, qsizetype index,
                                        const QString &accessor, const QString &returnType,
                                        const QList<QQmlJSAotVariable> &parameters = {});
    static QStringList
    generate_createBindingOnProperty(const QString &unitVarName, const QString &scope,
                                     qsizetype functionIndex, const QString &target,
                                     int propertyIndex, const QQmlJSMetaProperty &p,
                                     int valueTypeIndex, const QString &subTarget);
    static QString generate_qOverload(const QList<QQmlJSAotVariable> &parameters,
                                      const QString &overloaded);
    static QString generate_addressof(const QString &addressed);
    static QString generate_getPrivateClass(const QString &accessor, const QQmlJSMetaProperty &p);
    static QString generate_setIdValue(const QString &context, qsizetype index,
                                       const QString &accessor, const QString &idString);
};

#endif // CODEGENERATORUTIL_H
