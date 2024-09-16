// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "spreadformula.h"
#include "datamodel.h"
#include "spreadkey.h"
#include "spreadrole.h"
#include "spreadmodel.h"

bool Formula::includesLoop(SpreadModel *model, const DataModel *dataModel, QSet<int> *history) const
{
    if (m_operator == Operator::Invalid)
        return false;

    if (history == nullptr) {
        QSet<int> history;
        return includesLoop(model, dataModel, &history);
    }

    if (m_operator == Operator::Sum) {
        SpreadKey top_left = dataModel->getKey(m_cellIds.first);
        SpreadKey bottom_right = dataModel->getKey(m_cellIds.second);
        if (bottom_right.first < top_left.first)
            std::swap(top_left.first, bottom_right.first);
        if (bottom_right.second < top_left.second)
            std::swap(top_left.second, bottom_right.second);
        for (int row = top_left.first; row <= bottom_right.first; ++row) {
            for (int column = top_left.second; column <= bottom_right.second; ++column) {
                const int id = dataModel->getId(SpreadKey{row, column});
                if (history->find(id) != history->end())
                    return true;
                const QString edit_text = dataModel->getData(id, spread::Role::Edit).toString();
                const Formula formula = model->parseFormulaString(edit_text);
                if (!formula.isValid())
                    continue;
                auto it = history->insert(id);
                if (formula.includesLoop(model, dataModel, history))
                    return true;
                auto cit = spread::make_const(*history, it);
                history->erase(cit);
            }
        }
    } else {
        const int id_1 = m_cellIds.first;
        if (history->find(id_1) != history->end())
            return true;
        const QString edit_text = dataModel->getData(id_1, spread::Role::Edit).toString();
        const Formula formula = model->parseFormulaString(edit_text);
        if (!formula.isValid())
            return false;
        auto it = history->insert(id_1);
        if (formula.includesLoop(model, dataModel, history))
            return true;
        auto cit = spread::make_const(*history, it);
        history->erase(cit);

        if (m_operator != Operator::Assign) {
            const int id_2 = m_cellIds.second;
            if (history->find(id_2) != history->end())
                return true;
            const QString edit_text = dataModel->getData(id_2, spread::Role::Edit).toString();
            const Formula formula = model->parseFormulaString(edit_text);
            if (!formula.isValid())
                return false;
            auto it = history->insert(id_2);
            if (formula.includesLoop(model, dataModel, history))
                return true;
            auto cit = spread::make_const(*history, it);
            history->erase(cit);
        }
    }

    return false;
}
