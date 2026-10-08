// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4functiontable_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

void generateFunctionTable(Function *function, JSC::MacroAssemblerCodeRef *codeRef)
{
    Q_UNUSED(function);
    Q_UNUSED(codeRef);
}

void destroyFunctionTable(Function *function, JSC::MacroAssemblerCodeRef *codeRef)
{
    Q_UNUSED(function);
    Q_UNUSED(codeRef);
}

size_t exceptionHandlerSize()
{
    return 0;
}

} // QV4

QT_END_NAMESPACE
