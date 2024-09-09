// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "spreadformula.h"
#include "spreadcell.h"
#include "spreadrole.h"
#include "spreadmodel.h"

bool SpreadCell::isNull() const
{
    return !has(spread::Role::Display) && !has(spread::Role::Hightlight);
}

bool SpreadCell::has(int role) const
{
    switch (role) {
    case spread::Role::Display:
    case spread::Role::Edit:
        return !text.isNull() && !text.isEmpty();
    case spread::Role::Hightlight:
        return highlight;  // false highlight equals to no highlight set
    default:
        return false;
    }
}

void SpreadCell::set(int role, const QVariant &data)
{
    switch (role) {
    case spread::Role::Edit:
        text = data.toString();
        break;
    case spread::Role::Hightlight:
        highlight = data.toBool();
        break;
    default:
        break;
    }
}

QVariant SpreadCell::get(int role) const
{
    switch (role) {
    case spread::Role::Edit:
        return text;
    case spread::Role::Display: {
        const QString display_text = displayText();
        return display_text.isNull() ? QVariant{} : display_text;
    }
    case spread::Role::Hightlight:
        return highlight;
    default:
        return QVariant{};
    }
}

QString SpreadCell::displayText() const
{
    SpreadModel *model = SpreadModel::instance();
    const Formula formula = model->parseFormulaString(text);
    if (!formula.isValid())
        return text;
    if (formula.firstOperandId() <= 0)  // at least one arg should be available
        return "#ERROR!";
    if ((formula.firstOperandId() == id) || (formula.secondOperandId() == id))
        return "#ERROR!";  // found loop
    if (formula.includesLoop(model, model->dataModel()))
        return "#ERROR!";
    return model->formulaValueText(formula);
}
