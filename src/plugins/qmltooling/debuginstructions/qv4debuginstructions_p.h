// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4DEBUGINSTRUCTIONS_P_H
#define QV4DEBUGINSTRUCTIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qv4bytecodegenerator_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace Moth {

// Inserts Debug instructions into the generated code, so that the debugger can check for
// breakpoints when it executes them.
class DebugInstructions
{
public:
    static void insert(BytecodeGenerator *generator);

private:
    using I = BytecodeGenerator::I;

    DebugInstructions(BytecodeGenerator *generator) : m_generator(generator) {}
    void run();
    void appendDebug(int line, const I &before);

    BytecodeGenerator *m_generator;
};

} // namespace Moth
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4DEBUGINSTRUCTIONS_P_H
