// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "spreadformula.h"
#include "spreadmodel.h"
#include "spreadrole.h"

#include <QPoint>
#include <QRegularExpression>

int SpreadModel::columnNumberFromName(const QString &text)
{
    if (text.size() == 1)
        return getModelColumn(text[0].toLatin1() - 'A');
    else if (text.size() == 2)
        return getModelColumn((text[0].toLatin1() - 'A' + 1) * 26 + (text[1].toLatin1() - 'A'));
    return 0;
}

int SpreadModel::rowNumberFromName(const QString &text)
{
    return getModelRow(text.toInt() - 1);
}

SpreadModel *SpreadModel::instance()
{
    return s_instance;
}

SpreadModel *SpreadModel::create(QQmlEngine *, QJSEngine *)
{
    if (!s_instance)
        s_instance = new SpreadModel {nullptr};
    return s_instance;
}

void SpreadModel::update(int row, int column)
{
    if (!m_updateMutex.tryLock())
        return;
    column = getModelColumn(column);
    row = getModelRow(row);
    const QModelIndex index = this->index(row, column);
    emit dataChanged(index, index, {spread::Role::Display,});
    m_updateMutex.unlock();
}

void SpreadModel::clearHighlight()
{
    if (m_dataModel.empty())
        return;

    const auto [top_left, bottom_right] = m_dataModel.clearHighlight();

    emit dataChanged(index(top_left.first, top_left.second),
                     index(bottom_right.first, bottom_right.second),
                     {spread::Role::Hightlight,});
}

void SpreadModel::setHighlight(const QModelIndex &index, bool highlight)
{
    if (!index.isValid())
        return;
    if (m_dataModel.setHighlight(SpreadKey{index.row(), index.column()}, highlight))
        emit dataChanged(index, index, {spread::Role::Hightlight,});
}

int SpreadModel::rowCount(const QModelIndex &parent) const
{
    return m_size.first;
}

int SpreadModel::columnCount(const QModelIndex &parent) const
{
    return m_size.second;
}

QVariant SpreadModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant{};

    const int row = index.row();
    const int column = index.column();

    return m_dataModel.getData(SpreadKey{row, column}, role);
}

bool SpreadModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    const int row = index.row();
    const int column = index.column();

    if (m_dataModel.setData(SpreadKey{row, column}, value, role))
        emit dataChanged(index, index);

    return true;
}

bool SpreadModel::clearItemData(const QModelIndex &index)
{
    if (!index.isValid())
        return false;
    if (m_dataModel.clearData(SpreadKey{index.row(), index.column()})) {
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

Qt::ItemFlags SpreadModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

QHash<int, QByteArray> SpreadModel::roleNames() const
{
    return {{spread::Role::Display, "display"},
            {spread::Role::Edit, "edit"},
            {spread::Role::ColumnName, "columnName"},
            {spread::Role::RowName, "rowName"},
            {spread::Role::Hightlight, "highlight"}};

}

QVariant SpreadModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case spread::Role::ColumnName:
    case spread::Role::RowName:
        break;
    default:
        return QVariant{};
    }

    switch (orientation) {
    case Qt::Horizontal: {
        constexpr char A = 'A';
        const int view_section = getViewColumn(section);
        if (view_section < 26) {
            return QString{static_cast<char>(view_section + A)};
        } else {
            const int first = view_section / 26 - 1;
            const int second = view_section % 26;
            QString title{static_cast<char>(first + A)};
            title += static_cast<char>(second + A);
            return title;
        }
    }
    case Qt::Vertical: {
        return getViewRow(section) + 1;
    }
    }

    return QVariant{};
}

bool SpreadModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    if (count != 1)  // TODO: implement inserting more than 1 columns
        return false;

    beginInsertColumns(QModelIndex{}, column, column + count - 1);
    m_size.second += count;

    // update model
    m_dataModel.shiftColumns(column, count);

    QMap<int, int> old_columns;
    std::swap(old_columns, m_viewColumns);
    for (auto it = old_columns.begin(); it != old_columns.end(); ++it) {
        const int new_view_column = (it.value() < column) ? it.value() : it.value() + count;
        const int new_model_column = (it.key() < column) ? it.key() : it.key() + count;
        m_viewColumns.insert(new_model_column, new_view_column);
    }

    endInsertColumns();
    return true;
}

bool SpreadModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (count != 1)  // TODO: implement inserting more than 1 rows
        return false;

    beginInsertRows(QModelIndex{}, row, row + count - 1);
    m_size.first += count;

    // update model
    m_dataModel.shiftRows(row, count);

    endInsertRows();
    return true;
}

bool SpreadModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    if (count != 1)  // TODO: implement removing more than 1 columns
        return false;

    beginRemoveColumns(QModelIndex{}, column, column + count - 1);
    m_size.second -= count;

    // update model
    m_dataModel.removeColumnCells(column);
    m_dataModel.shiftColumns(column + 1, -count);

    endRemoveColumns();
    return true;
}

bool SpreadModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (count != 1)  // TODO: implement removing more than 1 rows
        return false;

    beginRemoveRows(QModelIndex{}, row, row + count - 1);
    m_size.first -= count;

    // update model
    m_dataModel.removeRowCells(row);
    m_dataModel.shiftRows(row + 1, -count);

    endRemoveRows();
    return true;
}

bool SpreadModel::clearItemData(const QModelIndexList &indexes)
{
    bool ok = true;
    for (const QModelIndex &index : indexes)
        ok &= clearItemData(index);
    return ok;
}

bool SpreadModel::removeColumns(QModelIndexList indexes)
{
    auto greater = [](const QModelIndex &lhs, const QModelIndex &rhs)
    {
        return lhs.column() > rhs.column();
    };
    std::sort(indexes.begin(), indexes.end(), greater);
    for (const QModelIndex &index : indexes)
        removeColumn(index.column());
    return true;
}

bool SpreadModel::removeRows(QModelIndexList indexes)
{
    auto greater = [](const QModelIndex &lhs, const QModelIndex &rhs)
    {
        return lhs.row() > rhs.row();
    };
    std::sort(indexes.begin(), indexes.end(), greater);
    for (const QModelIndex &index : indexes)
        removeRow(index.row());
    return true;
}

void SpreadModel::mapColumn(int model, int view)
{
    if (model == view)
        m_viewColumns.remove(model);
    else
        m_viewColumns[model] = view;
    emit headerDataChanged(Qt::Horizontal, model, view);
}

void SpreadModel::mapRow(int model, int view)
{
    if (model == view)
        m_viewRows.remove(model);
    else
        m_viewRows[model] = view;
    emit headerDataChanged(Qt::Vertical, model, view);
}

Formula SpreadModel::parseFormulaString(const QString &qText)
{
    if (qText.isEmpty())
        return Formula{};

    QRegularExpression pattern_re;

    // is formula
    pattern_re.setPattern("^\\s*=.*");
    QRegularExpressionMatch match = pattern_re.match(qText);
    if (!match.hasMatch())
        return Formula{};

    // is Assignment: e.g. =A1
    pattern_re.setPattern("^\\s*=\\s*([a-zA-Z]+)([1-9][0-9]*)\\s*$");
    match = pattern_re.match(qText);
    if (match.hasMatch()) {
        const QString column_label = match.captured(1);
        const QString row_label = match.captured(2);
        const int column = columnNumberFromName(column_label);
        const int row = rowNumberFromName(row_label);
        const int cell_id = m_dataModel.createId(SpreadKey{row, column});
        return Formula::create(Formula::Operator::Assign, cell_id, 0);
    }

    // is Addition: e.g. =A1+A2
    pattern_re.setPattern("^\\s*=\\s*([a-zA-Z]+)([1-9][0-9]*)\\s*\\+\\s*([a-zA-Z]+)([1-9][0-9]*)\\s*$");
    match = pattern_re.match(qText);
    if (match.hasMatch()) {
        // first argument
        QString column_label = match.captured(1);
        QString row_label = match.captured(2);
        int column = columnNumberFromName(column_label);
        int row = rowNumberFromName(row_label);
        const int cell_id_1 = m_dataModel.createId(SpreadKey{row, column});
        // second argument
        column_label = match.captured(3);
        row_label = match.captured(4);
        column = columnNumberFromName(column_label);
        row = rowNumberFromName(row_label);
        const int cell_id_2 = m_dataModel.createId(SpreadKey{row, column});
        // create formula
        return Formula::create(Formula::Operator::Add, cell_id_1, cell_id_2);
    }

    // is Subtraction: e.g. =A1-A2
    pattern_re.setPattern("^\\s*=\\s*([a-zA-Z]+)([1-9][0-9]*)\\s*\\-\\s*([a-zA-Z]+)([1-9][0-9]*)\\s*$");
    match = pattern_re.match(qText);
    if (match.hasMatch()) {
        // first argument
        QString column_label = match.captured(1);
        QString row_label = match.captured(2);
        int column = columnNumberFromName(column_label);
        int row = rowNumberFromName(row_label);
        const int cell_id_1 = m_dataModel.createId(SpreadKey{row, column});
        // second argument
        column_label = match.captured(3);
        row_label = match.captured(4);
        column = columnNumberFromName(column_label);
        row = rowNumberFromName(row_label);
        const int cell_id_2 = m_dataModel.createId(SpreadKey{row, column});
        // create formula
        return Formula::create(Formula::Operator::Sub, cell_id_1, cell_id_2);
    }

    // is Multiply: e.g. =A1*A2
    pattern_re.setPattern("^\\s*=\\s*([a-zA-Z]+)([1-9][0-9]*)\\s*\\*\\s*([a-zA-Z]+)([1-9][0-9]*)\\s*$");
    match = pattern_re.match(qText);
    if (match.hasMatch()) {
        // first argument
        QString column_label = match.captured(1);
        QString row_label = match.captured(2);
        int column = columnNumberFromName(column_label);
        int row = rowNumberFromName(row_label);
        const int cell_id_1 = m_dataModel.createId(SpreadKey{row, column});
        // second argument
        column_label = match.captured(3);
        row_label = match.captured(4);
        column = columnNumberFromName(column_label);
        row = rowNumberFromName(row_label);
        const int cell_id_2 = m_dataModel.createId(SpreadKey{row, column});
        // create formula
        return Formula::create(Formula::Operator::Mul, cell_id_1, cell_id_2);
    }

    // is Division: e.g. =A1/A2
    pattern_re.setPattern("^\\s*=\\s*([a-zA-Z]+)([1-9][0-9]*)\\s*\\/\\s*([a-zA-Z]+)([1-9][0-9]*)\\s*$");
    match = pattern_re.match(qText);
    if (match.hasMatch()) {
        // first argument
        QString column_label = match.captured(1);
        QString row_label = match.captured(2);
        int column = columnNumberFromName(column_label);
        int row = rowNumberFromName(row_label);
        const int cell_id_1 = m_dataModel.createId(SpreadKey{row, column});
        // second argument
        column_label = match.captured(3);
        row_label = match.captured(4);
        column = columnNumberFromName(column_label);
        row = rowNumberFromName(row_label);
        const int cell_id_2 = m_dataModel.createId(SpreadKey{row, column});
        // create formula
        return Formula::create(Formula::Operator::Div, cell_id_1, cell_id_2);
    }

    // is Summation: e.g. =SUM A1:A2
    pattern_re.setPattern("^\\s*=\\s*[Ss][Uu][Mm]\\s+([a-zA-Z]+)([1-9][0-9]*)\\s*\\:\\s*([a-zA-Z]+)([1-9][0-9]*)\\s*$");
    match = pattern_re.match(qText);
    if (match.hasMatch()) {
        // first argument
        QString column_label = match.captured(1);
        QString row_label = match.captured(2);
        int column = columnNumberFromName(column_label);
        int row = rowNumberFromName(row_label);
        const int cell_id_1 = m_dataModel.createId(SpreadKey{row, column});
        // second argument
        column_label = match.captured(3);
        row_label = match.captured(4);
        column = columnNumberFromName(column_label);
        row = rowNumberFromName(row_label);
        const int cell_id_2 = m_dataModel.createId(SpreadKey{row, column});
        // create formula
        return Formula::create(Formula::Operator::Sum, cell_id_1, cell_id_2);
    }

    return Formula{};
}

QString SpreadModel::formulaValueText(const Formula &formula)
{
    switch (formula.getOperator()) {
    case Formula::Operator::Assign: {
        const QVariant value = m_dataModel.getData(formula.firstOperandId(), spread::Role::Display);
        return value.isNull() ? QString{} : value.toString();
    }
    case Formula::Operator::Add: {
        const QVariant value_1 = m_dataModel.getData(formula.firstOperandId(), spread::Role::Display);
        const QVariant value_2 = m_dataModel.getData(formula.secondOperandId(), spread::Role::Display);
        if (value_1.isNull() && value_2.isNull())
            return "0";
        // check int values
        bool is_int_1 = true;
        bool is_int_2 = true;
        const int int_1 = value_1.isNull() ? 0 : value_1.toInt(&is_int_1);
        const int int_2 = value_2.isNull() ? 0 : value_2.toInt(&is_int_2);
        if (is_int_1 && is_int_2)
            return QString::number(int_1 + int_2);
        // check double values
        bool is_double_1 = true;
        bool is_double_2 = true;
        const double double_1 = value_1.isNull() ? 0 : value_1.toDouble(&is_double_1);
        const double double_2 = value_2.isNull() ? 0 : value_2.toDouble(&is_double_2);
        if (is_double_1 && is_double_2)
            return QString::number(double_1 + double_2);
        return "#ERROR!";
    }
    case Formula::Operator::Sub: {
        const QVariant value_1 = m_dataModel.getData(formula.firstOperandId(), spread::Role::Display);
        const QVariant value_2 = m_dataModel.getData(formula.secondOperandId(), spread::Role::Display);
        if (value_1.isNull() && value_2.isNull())
            return "0";
        // check int values
        bool is_int_1 = true;
        bool is_int_2 = true;
        const int int_1 = value_1.isNull() ? 0 : value_1.toInt(&is_int_1);
        const int int_2 = value_2.isNull() ? 0 : value_2.toInt(&is_int_2);
        if (is_int_1 && is_int_2)
            return QString::number(int_1 - int_2);
        // check double values
        bool is_double_1 = true;
        bool is_double_2 = true;
        const double double_1 = value_1.isNull() ? 0 : value_1.toDouble(&is_double_1);
        const double double_2 = value_2.isNull() ? 0 : value_2.toDouble(&is_double_2);
        if (is_double_1 && is_double_2)
            return QString::number(double_1 - double_2);
        return "#ERROR!";
    }
    case Formula::Operator::Mul: {
        const QVariant value_1 = m_dataModel.getData(formula.firstOperandId(), spread::Role::Display);
        const QVariant value_2 = m_dataModel.getData(formula.secondOperandId(), spread::Role::Display);
        if (value_1.isNull() || value_2.isNull())
            return "0";
        // check int values
        bool is_int_1 = true;
        bool is_int_2 = true;
        const int int_1 = value_1.toInt(&is_int_1);
        const int int_2 = value_2.toInt(&is_int_2);
        if (is_int_1 && is_int_2)
            return QString::number(int_1 * int_2);
        // check double values
        bool is_double_1 = true;
        bool is_double_2 = true;
        const double double_1 = value_1.toDouble(&is_double_1);
        const double double_2 = value_2.toDouble(&is_double_2);
        if (is_double_1 && is_double_2)
            return QString::number(double_1 * double_2);
        return "#ERROR!";
    }
    case Formula::Operator::Div: {
        const QVariant value_1 = m_dataModel.getData(formula.firstOperandId(), spread::Role::Display);
        const QVariant value_2 = m_dataModel.getData(formula.secondOperandId(), spread::Role::Display);
        if (value_1.isNull() && value_2.isNull())
            return "#ERROR!";
        // check int values
        bool is_int_1 = true;
        bool is_int_2 = true;
        const int int_1 = value_1.isNull() ? 0 : value_1.toInt(&is_int_1);
        const int int_2 = value_2.isNull() ? 0 : value_2.toInt(&is_int_2);
        if (is_int_1 && is_int_2) {
            if (int_2 == 0)
                return "#ERROR!";
            return QString::number(int_1 / int_2);
        }
        // check double values
        bool is_double_1 = true;
        bool is_double_2 = true;
        const double double_1 = value_1.isNull() ? 0 : value_1.toDouble(&is_double_1);
        const double double_2 = value_2.isNull() ? 0 : value_2.toDouble(&is_double_2);
        if (is_double_1 && is_double_2) {
            if (double_2 == 0)
                return "#ERROR!";
            return QString::number(double_1 / double_2);
        }
        return "#ERROR!";
    }
    case Formula::Operator::Sum: {
        SpreadKey top_left = m_dataModel.getKey(formula.firstOperandId());
        SpreadKey bottom_right = m_dataModel.getKey(formula.secondOperandId());
        top_left.first = getViewRow(top_left.first);
        top_left.second = getViewColumn(top_left.second);
        bottom_right.first = getViewRow(bottom_right.first);
        bottom_right.second = getViewColumn(bottom_right.second);
        if (bottom_right.first < top_left.first)
            std::swap(top_left.first, bottom_right.first);
        if (bottom_right.second < top_left.second)
            std::swap(top_left.second, bottom_right.second);
        double sum = 0;
        for (int row = top_left.first; row <= bottom_right.first; ++row) {
            for (int column = top_left.second; column <= bottom_right.second; ++column) {
                const int model_row = getModelRow(row);
                const int model_column = getModelColumn(column);
                const SpreadKey key {model_row, model_column};
                const QVariant value = m_dataModel.getData(key, spread::Role::Display);
                if (value.isNull())
                    continue;
                bool is_double = false;
                const double d = value.toDouble(&is_double);
                if (is_double)
                    sum += d;
                else
                    return "#ERROR!";
            }
        }
        return QString::number(sum);
    }
    default:
        break;
    }
    return QString{};
}

int SpreadModel::getViewColumn(int modelColumn) const
{
    auto it = m_viewColumns.find(modelColumn);
    return (it != m_viewColumns.end()) ? it.value() : modelColumn;
}

int SpreadModel::getModelColumn(int viewColumn) const
{
    auto find_view_column = [viewColumn](const auto &item) {
        return item == viewColumn;
    };
    auto it = std::find_if(m_viewColumns.begin(), m_viewColumns.end(), find_view_column);
    return it != m_viewColumns.end() ? it.key() : viewColumn;
}

int SpreadModel::getViewRow(int modelRow) const
{
    auto it = m_viewRows.find(modelRow);
    return (it != m_viewRows.end()) ? it.value() : modelRow;
}

int SpreadModel::getModelRow(int viewRow) const
{
    auto find_view_row = [viewRow](const auto &item){
        return item == viewRow;
    };
    auto it = std::find_if(m_viewRows.begin(), m_viewRows.end(), find_view_row);
    return (it != m_viewRows.end()) ? it.key() : viewRow;
}

void SpreadSelectionModel::toggleColumn(int column)
{
    isColumnSelected(column) ? deselectColumn(column) : selectColumn(column, false);
}

void SpreadSelectionModel::deselectColumn(int column)
{
    const QAbstractItemModel *model = this->model();
    const QModelIndex first = model->index(0, column);
    const QModelIndex last = model->index(model->rowCount() - 1, column);
    QModelIndexList selectedRows = this->selectedRows(column);
    if (selectedRows.empty()) {
        select(QItemSelection{first, last}, SelectionFlag::Deselect);
        return;
    }

    auto topToBottom = [](const QModelIndex &lhs, const QModelIndex &rhs) -> bool
    {
        return lhs.row() < rhs.row();
    };
    std::sort(selectedRows.begin(), selectedRows.end(), topToBottom);

    QModelIndex index = first;
    for (const QModelIndex &selectedRow : selectedRows) {
        if (index.row() < selectedRow.row())
            select(QItemSelection{index, model->index(selectedRow.row() - 1, column)},
                   SelectionFlag::Deselect);
        index = model->index(selectedRow.row() + 1, column);
    }
    if (index.row() <= last.row())
        select(QItemSelection{index, last}, SelectionFlag::Deselect);
}

void SpreadSelectionModel::selectColumn(int column, bool clear)
{
    if (clear)
        this->clear();
    const QAbstractItemModel *model = this->model();
    const QModelIndex first = model->index(0, column);
    const QModelIndex last = model->index(model->rowCount() - 1, column);
    select(QItemSelection{first, last}, SelectionFlag::Select);
}

void SpreadSelectionModel::toggleRow(int row)
{
    isRowSelected(row) ? deselectRow(row) : selectRow(row, false);
}

void SpreadSelectionModel::deselectRow(int row)
{
    const QAbstractItemModel *model = this->model();
    const QModelIndex first = model->index(row, 0);
    const QModelIndex last = model->index(row, model->columnCount() - 1);
    QModelIndexList selectedColumns = this->selectedColumns(row);
    if (selectedColumns.empty()) {
        select(QItemSelection{first, last}, SelectionFlag::Deselect);
        return;
    }

    auto leftToRight = [](const QModelIndex &lhs, const QModelIndex &rhs) -> bool
    {
        return lhs.column() < rhs.column();
    };
    std::sort(selectedColumns.begin(), selectedColumns.end(), leftToRight);

    QModelIndex index = first;
    for (const QModelIndex &selectedColumn : selectedColumns) {
       if (index.column() < selectedColumn.column())
           select(QItemSelection{index, model->index(row, selectedColumn.column() - 1)},
                  SelectionFlag::Deselect);
       index = model->index(row, selectedColumn.column() + 1);
    }
    if (index.column() <= last.column())
       select(QItemSelection{index, last}, SelectionFlag::Deselect);
}

void SpreadSelectionModel::selectRow(int row, bool clear)
{
    if (clear)
        this->clear();
    const QAbstractItemModel *model = this->model();
    const QModelIndex first = model->index(row, 0);
    const QModelIndex last = model->index(row, model->columnCount() - 1);
    select(QItemSelection{first, last}, SelectionFlag::Select);
}

void SpreadSelectionModel::setBehavior(Behavior behavior)
{
    if (behavior == m_behavior)
        return;
    m_behavior = behavior;
    emit behaviorChanged();
}

void HeaderSelectionModel::setCurrent(int current)
{
    switch (m_orientation) {
    case Qt::Horizontal:
        QItemSelectionModel::setCurrentIndex(model()->index(0, current), SelectionFlag::Current);
        break;
    case Qt::Vertical:
        QItemSelectionModel::setCurrentIndex(model()->index(current, 0), SelectionFlag::Current);
        break;
    default:
        break;
    }
}

void HeaderSelectionModel::setSelectionModel(SpreadSelectionModel *selectionModel)
{
    if (selectionModel == m_selectionModel)
        return;
    if (m_selectionModel)
        disconnect(m_selectionModel);
    m_selectionModel = selectionModel;
    if (m_selectionModel)
        connect(m_selectionModel, &SpreadSelectionModel::selectionChanged, this,
                &HeaderSelectionModel::onSelectionChanged);
    emit selectionModelChanged();
}

void HeaderSelectionModel::setOrientation(Qt::Orientation orientation)
{
    if (orientation == m_orientation)
        return;
    m_orientation = orientation;
    emit orientationChanged();
}

void HeaderSelectionModel::onSelectionChanged(const QItemSelection &selected,
                                              const QItemSelection &deselected)
{
    const QAbstractItemModel *model = this->model();
    for (const QModelIndex &index : selected.indexes()) {
        switch (m_orientation) {
        case Qt::Horizontal:
            if (m_selectionModel->isColumnSelected(index.column()))
                select(model->index(0, index.column()), SelectionFlag::Select);
            break;
        case Qt::Vertical:
            if (m_selectionModel->isRowSelected(index.row()))
                select(model->index(index.row(), 0), SelectionFlag::Select);
            break;
        }
    }
    for (const QModelIndex &index : deselected.indexes()) {
        switch (m_orientation) {
        case Qt::Horizontal:
            if (!m_selectionModel->isColumnSelected(index.column()))
                select(model->index(0, index.column()), SelectionFlag::Deselect);
            break;
        case Qt::Vertical:
            if (!m_selectionModel->isRowSelected(index.row()))
                select(model->index(index.row(), 0), SelectionFlag::Deselect);
            break;
        }
    }
}
