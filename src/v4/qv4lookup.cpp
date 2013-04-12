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
#include "qv4lookup.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

void QQmlJS::VM::Lookup::lookupName(QQmlJS::VM::Lookup *l, ExecutionContext *ctx, QQmlJS::VM::Value *result, const QQmlJS::VM::Value &object)
{
    Value res;
    if (Object *o = object.asObject()) {
        PropertyAttributes attrs;
        Property *p = l->lookup(o, &attrs);
        if (p)
            res = attrs.isData() ? p->value : o->getValue(ctx, p, attrs);
        else
            res = Value::undefinedValue();
    } else {
        if (Managed *m = object.asManaged()) {
            res = m->get(ctx, l->name);
        } else {
            o = __qmljs_convert_to_object(ctx, object);
            res = o->get(ctx, l->name);
        }
    }
    if (result)
        *result = res;
}

}
}

QT_END_NAMESPACE
