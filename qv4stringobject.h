/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4STRINGOBJECT_P_H
#define QV4STRINGOBJECT_P_H

#include "qv4object.h"
#include "qv4functionobject.h"
#include <QtCore/qnumeric.h>

namespace QQmlJS {
namespace VM {

struct StringObject: Object {
    Value value;
    PropertyDescriptor tmpProperty;
    StringObject(ExecutionContext *ctx, const Value &value);
    virtual QString className() { return QStringLiteral("String"); }

    PropertyDescriptor *getIndex(ExecutionContext *ctx, uint index);
};

struct StringCtor: FunctionObject
{
    StringCtor(ExecutionContext *scope);

    virtual Value construct(ExecutionContext *ctx);
    virtual Value call(ExecutionContext *ctx);
};

struct StringPrototype: StringObject
{
    StringPrototype(ExecutionContext *ctx): StringObject(ctx, Value::fromString(ctx, QString())) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static QString getThisString(ExecutionContext *ctx);

    static Value method_toString(ExecutionContext *ctx);
    static Value method_valueOf(ExecutionContext *ctx);
    static Value method_charAt(ExecutionContext *ctx);
    static Value method_charCodeAt(ExecutionContext *ctx);
    static Value method_concat(ExecutionContext *ctx);
    static Value method_indexOf(ExecutionContext *ctx);
    static Value method_lastIndexOf(ExecutionContext *ctx);
    static Value method_localeCompare(ExecutionContext *ctx);
    static Value method_match(ExecutionContext *ctx);
    static Value method_replace(ExecutionContext *ctx);
    static Value method_search(ExecutionContext *ctx);
    static Value method_slice(ExecutionContext *ctx);
    static Value method_split(ExecutionContext *ctx);
    static Value method_substr(ExecutionContext *ctx);
    static Value method_substring(ExecutionContext *ctx);
    static Value method_toLowerCase(ExecutionContext *ctx);
    static Value method_toLocaleLowerCase(ExecutionContext *ctx);
    static Value method_toUpperCase(ExecutionContext *ctx);
    static Value method_toLocaleUpperCase(ExecutionContext *ctx);
    static Value method_fromCharCode(ExecutionContext *ctx);
    static Value method_trim(ExecutionContext *ctx);
};

} // end of namespace VM
} // end of namespace QQmlJS

#endif // QV4ECMAOBJECTS_P_H
