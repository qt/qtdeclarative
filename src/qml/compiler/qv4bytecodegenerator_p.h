/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QV4BYTECODEGENERATOR_P_H
#define QV4BYTECODEGENERATOR_P_H

#include <private/qv4instr_moth_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {
struct Function;
}
namespace Moth {

class BytecodeGenerator {
public:
    BytecodeGenerator(IR::Function *function)
        : function(function) {}

    struct CodeRef {
        int instructionIndex;
    };

    struct Label {
        void link(CodeRef r) {
            linkedInstructions.append(r.instructionIndex);
        }

        int index;
        QVector<int> linkedInstructions;
    };

    Label label() {
        Label l;
        l.index = labels.size();
        return l;
    }

    template<int InstrT>
    CodeRef addInstruction(const InstrData<InstrT> &data)
    {
        Instr genericInstr;
        InstrMeta<InstrT>::setData(genericInstr, data);
        return { addInstructionHelper(InstrMeta<InstrT>::Size, genericInstr) };
    }

    unsigned newTemp();


    QByteArray finalize();

private:
    int addInstructionHelper(uint size, const Instr &i) {
        int pos = instructions.size();
        instructions.append({size, i});
        return pos;
    }

    struct I {
        uint size;
        Instr instr;
    };

    QVector<I> instructions;

    QVector<Label> labels;
    IR::Function *function; // ### remove me at some point
};

}
}

QT_END_NAMESPACE

#endif
