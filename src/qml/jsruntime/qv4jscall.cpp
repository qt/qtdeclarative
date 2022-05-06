/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qv4jscall_p.h"

#include <private/qqmlengine_p.h>
#include <private/qv4qobjectwrapper_p.h>

QT_BEGIN_NAMESPACE

/*! \internal

    Sets the arguments of JSCallData from type erased \a args based on type
    information provided by \a types
 */
void QV4::populateJSCallArguments(ExecutionEngine *v4, JSCallArguments &jsCall,
                                  int argc, void **args, const QMetaType *types)
{
    for (int ii = 0; ii < argc; ++ii)
        jsCall.args[ii] = v4->metaTypeToJS(types[ii], args[ii + 1]);
}

QT_END_NAMESPACE
