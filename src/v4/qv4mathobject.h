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
#ifndef QV4MATHOBJECT_H
#define QV$MATHOBJECT_H

#include "qv4object.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

struct MathObject: Object
{
    MathObject(ExecutionContext *ctx);
    virtual QString className() { return QStringLiteral("Math"); }

    static Value method_abs(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_acos(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_asin(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_atan(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_atan2(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_ceil(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_cos(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_exp(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_floor(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_log(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_max(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_min(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_pow(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_random(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_round(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_sin(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_sqrt(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_tan(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
};

} // namespace VM
} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
