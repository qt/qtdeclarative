/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4jscall_p.h"

#include <private/qqmlengine_p.h>
#include <private/qv4qobjectwrapper_p.h>

QT_BEGIN_NAMESPACE

/*! \internal

    Sets the arguments of JSCallData from type erased \a args based on type
    information provided by \a types
 */
void QV4::populateJSCallArguments(QQmlEnginePrivate *ep, ExecutionEngine *v4, JSCallData &jsCall,
                                  void **args, int *types)
{
    const int argCount = types ? types[0] : 0;
    for (int ii = 0; ii < argCount; ++ii) {
        int type = types[ii + 1];
        //### ideally we would use metaTypeToJS, however it currently gives different results
        //    for several cases (such as QVariant type and QObject-derived types)
        // args[ii] = v4->metaTypeToJS(type, args[ii + 1]);
        if (type == qMetaTypeId<QJSValue>()) {
            jsCall.args[ii] = QJSValuePrivate::convertToReturnedValue(
                    v4, *reinterpret_cast<QJSValue *>(args[ii + 1]));
        } else if (type == QMetaType::QVariant) {
            jsCall.args[ii] = v4->fromVariant(*((QVariant *)args[ii + 1]));
        } else if (type == QMetaType::Int) {
            //### optimization. Can go away if we switch to metaTypeToJS, or be expanded otherwise
            jsCall.args[ii] = QV4::Value::fromInt32(*reinterpret_cast<const int *>(args[ii + 1]));
        } else if (ep->isQObject(type)) {
            if (!*reinterpret_cast<void *const *>(args[ii + 1]))
                jsCall.args[ii] = QV4::Value::nullValue();
            else
                jsCall.args[ii] = QV4::QObjectWrapper::wrap(
                        v4, *reinterpret_cast<QObject *const *>(args[ii + 1]));
        } else {
            jsCall.args[ii] = v4->fromVariant(QVariant(QMetaType(type), args[ii + 1]));
        }
    }
}

QT_END_NAMESPACE
