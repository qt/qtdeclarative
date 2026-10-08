// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4jscall_p.h"

#include <QtQml/qqmlinfo.h>

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

void QV4::warnAboutCoercionToVoid(
        ExecutionEngine *engine, const Value &value, CoercionProblem problem)
{
    auto log = qCritical().nospace().noquote();
    if (const CppStackFrame *frame = engine->currentStackFrame)
        log << frame->source() << ':' << frame->lineNumber() << ": ";
    log << value.toQStringNoThrow()
        << " should be coerced to void because";
    switch (problem) {
    case InsufficientAnnotation:
        log << " the function called is insufficiently annotated.";
        break;
    case InvalidListType:
        log << " the target type, a list of unknown elements, cannot be resolved.";
        break;
    }

    log << " The original value is retained. This will change in a future version of Qt.";
}


QT_END_NAMESPACE
