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

#include "qmljs_objects.h"

namespace QQmlJS {

namespace VM {

struct MathObject: Object
{
    MathObject(ExecutionContext *ctx);

    static Value method_abs(ExecutionContext *ctx);
    static Value method_acos(ExecutionContext *ctx);
    static Value method_asin(ExecutionContext *ctx);
    static Value method_atan(ExecutionContext *ctx);
    static Value method_atan2(ExecutionContext *ctx);
    static Value method_ceil(ExecutionContext *ctx);
    static Value method_cos(ExecutionContext *ctx);
    static Value method_exp(ExecutionContext *ctx);
    static Value method_floor(ExecutionContext *ctx);
    static Value method_log(ExecutionContext *ctx);
    static Value method_max(ExecutionContext *ctx);
    static Value method_min(ExecutionContext *ctx);
    static Value method_pow(ExecutionContext *ctx);
    static Value method_random(ExecutionContext *ctx);
    static Value method_round(ExecutionContext *ctx);
    static Value method_sin(ExecutionContext *ctx);
    static Value method_sqrt(ExecutionContext *ctx);
    static Value method_tan(ExecutionContext *ctx);
};

} // namespace VM
} // namespace QQmlJS

#endif // QMLJS_OBJECTS_H
