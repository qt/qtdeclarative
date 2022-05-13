// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
