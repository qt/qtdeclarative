// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SPREADFORMULA_H
#define SPREADFORMULA_H

#include <QSet>

class DataModel;
class SpreadModel;

struct Formula {
    enum class Operator {
        Invalid = 0,
        Assign,
        Add,
        Sub,
        Div,
        Mul,
        Sum,
    };

    static Formula create(Operator op, int arg1, int arg2) {
        Formula formula;
        formula.m_operator = op;
        formula.m_cellIds.first = arg1;
        formula.m_cellIds.second = arg2;
        return formula;
    }

    bool isValid() const { return m_operator != Operator::Invalid; }
    int firstOperandId() const { return m_cellIds.first; }
    int secondOperandId() const { return m_cellIds.second; }
    Operator getOperator() const { return m_operator; }

    bool includesLoop(SpreadModel *model, const DataModel *dataModel, QSet<int> *history = nullptr) const;

private:
    Operator m_operator = Operator::Invalid;
    std::pair<int, int> m_cellIds = {0, 0};
};

#endif // SPREADFORMULA_H
