// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef OPTIONS_H
#define OPTIONS_H

namespace JSC {

struct Options {
    static bool showDisassembly();
    static bool showDFGDisassembly() { return true; }
    static bool zeroStackFrame() { return true; }
    static bool dumpCompiledRegExpPatterns() { return false; }
};

}

#endif // MASM_STUBS/OPTIONS_H
