// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//! [file]
#include "backend.h"

Backend::Backend(QJSEngine *engine) :
    mEngine(engine)
{
}

bool Backend::load()
{
    // Do some loading here...

    const QJSValue module = mEngine->importModule(":/script.mjs");
    if (module.isError()) {
        qWarning() << "Error loading script.mjs:" << module.toString();
        return false;
    }

    const QJSValue function = module.property("backendStatusUpdate");
    if (!function.isCallable()) {
        qWarning() << "backendStatusUpdate script function is not callable!";
        return false;
    }

    const QJSValue functionResult = function.call(QJSValueList() << Loaded);
    if (functionResult.isError()) {
        qWarning() << "backendStatusUpdate script function had errors:" << functionResult.toString();
        return false;
    }

    return true;
}
//! [file]
