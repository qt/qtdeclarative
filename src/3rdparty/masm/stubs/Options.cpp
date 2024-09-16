// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "config.h"
#include "Options.h"

#include <QByteArray>

namespace JSC {

bool Options::showDisassembly()
{
    static const bool showCode = qEnvironmentVariableIsSet("QV4_SHOW_ASM");
    return showCode;
}

}
