/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qqmltablemodel_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTableModel, "qt.qml.tablemodel")

/*!
    \qmltype TableModel
    \instantiates QQmlTableModel
    \inqmlmodule Qt.labs.qmlmodels
    \brief Encapsulates a simple table model.
    \since 5.14

    The TableModel type stores JavaScript/JSON objects as data for a table
    model that can be used with \l TableView. It is intended to support
    very simple models without requiring the creation of a custom
    QAbstractTableModel subclass in C++.

    \snippet qml/tablemodel/fruit-example-simpledelegate.qml file

    The model's initial row data is set with either the \l rows property or by
    calling \l appendRow(). Each column in the model is specified by declaring
    a \l TableModelColumn instance, where the order of each instance determines
    its column index. Once the model's \l Component::completed() signal has been
    emitted, the columns and roles will have been established and are then
    fixed for the lifetime of the model.

    To access a specific row, the \l getRow() function can be used.
    It's also possible to access the model's JavaScript data
    directly via the \l rows property, but it is not possible to
    modify the model data this way.

    To add new rows, use \l appendRow() and \l insertRow(). To modify
    existing rows, use \l setRow(), \l moveRow(), \l removeRow(), and
    \l clear().

    It is also possible to modify the model's data via the delegate,
    as shown in the example above:

    \snippet qml/tablemodel/fruit-example-simpledelegate.qml delegate

    If the type of the data at the modified role does not match the type of the
    data that is set, it will be automatically converted via
    \l {QVariant::canConvert()}{QVariant}.

    \section1 Supported Row Data Structures

    TableModel is designed to work with JavaScript/JSON data, where each row
    is a simple key-pair object:

    \code
    {
        // Each property is one cell/column.
        checked: false,
        amount: 1,
        fruitType: "Apple",
        fruitName: "Granny Smith",
        fruitPrice: 1.50
    },
    // ...
    \endcode

    As model manipulation in Qt is done via row and column indices,
    and because object keys are unordered, each column must be specified via
    TableModelColumn. This allows mapping Qt's built-in roles to any property
    in each row object.

    Complex row structures are supported, but with limited functionality.
    As TableModel has no way of knowing how each row is structured,
    it cannot manipulate it. As a consequence of this, the copy of the
    model data that TableModel has stored in \l rows is not kept in sync
    with the source data that was set in QML. For these reasons, TableModel
    relies on the user to handle simple data manipulation.

    For example, suppose you wanted to have several roles per column. One way
    of doing this is to use a data source where each row is an array and each
    cell is an object. To use this data source with TableModel, define a
    getter and setter:

    \code
    TableModel {
        TableModelColumn {
            display: function(modelIndex) { return rows[modelIndex.row][0].checked }
            setDisplay: function(modelIndex, cellData) { rows[modelIndex.row][0].checked = cellData }
        }
        // ...

        rows: [
            [
                { checked: false, checkable: true },
                { amount: 1 },
                { fruitType: "Apple" },
                { fruitName: "Granny Smith" },
                { fruitPrice: 1.50 }
            ]
            // ...
        ]
    }
    \endcode

    The row above is one example of a complex row.

    \note Row manipulation functions such as \l appendRow(), \l removeRow(),
    etc. are not supported when using complex rows.

    \section1 Using DelegateChooser with TableModel

    For most real world use cases, it is recommended to use DelegateChooser
    as the delegate of a TableView that uses TableModel. This allows you to
    use specific roles in the relevant delegates. For example, the snippet
    above can be rewritten to use DelegateChooser like so:

    \snippet qml/tablemodel/fruit-example-delegatechooser.qml file

    The most specific delegates are declared first: the columns at index \c 0
    and \c 1 have \c bool and \c integer data types, so they use a
    \l [QtQuickControls2]{CheckBox} and \l [QtQuickControls2]{SpinBox},
    respectively. The remaining columns can simply use a
    \l [QtQuickControls2]{TextField}, and so that delegate is declared
    last as a fallback.

    \sa TableModelColumn, TableView, QAbstractTableModel
*/

QQmlTableModel::QQmlTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QQmlTableModel::~QQmlTableModel()
{
}

/*!
    \qmlproperty object TableModel::rows

    This property holds the model data in the form of an array of rows:

    \snippet qml/tablemodel/fruit-example-simpledelegate.qml rows

    \sa getRow(), setRow(), moveRow(), appendRow(), insertRow(), clear(), rowCount, columnCount
*/
QVariant QQmlTableModel::rows() const
{
    return mRows;
}

void QQmlTableModel::setRows(const QVariant &rows)
{
    if (rows.userType() != qMetaTypeId<QJSValue>()) {
        qmlWarning(this) << "setRows(): \"rows\" must be an array; actual type is " << rows.typeName();
        return;
    }

    const QJSValue rowsAsJSValue = rows.value<QJSValue>();
    const QVariantList rowsAsVariantList = rowsAsJSValue.toVariant().toList();
    if (rowsAsVariantList == mRows) {
        // No change.
        return;
    }

    if (!componentCompleted) {
        // Store the rows until we can call doSetRows() after component completion.
        mRows = rowsAsVariantList;
        return;
    }

    doSetRows(rowsAsVariantList);
}

void QQmlTableModel::doSetRows(const QVariantList &rowsAsVariantList)
{
    Q_ASSERT(componentCompleted);

    // By now, all TableModelColumns should have been set.
    if (mColumns.isEmpty()) {
        qmlWarning(this) << "No TableModelColumns were set; model will be empty";
        return;
    }

    const bool firstTimeValidRowsHaveBeenSet = mColumnMetadata.isEmpty();
    if (!firstTimeValidRowsHaveBeenSet) {
        // This is not the first time rows have been set; validate each one.
        for (int rowIndex = 0; rowIndex < rowsAsVariantList.size(); ++rowIndex) {
            // validateNewRow() expects a QVariant wrapping a QJSValue, so to
            // simplify the code, just create one here.
            const QVariant row = QVariant::fromValue(rowsAsVariantList.at(rowIndex));
            if (!validateNewRow("setRows()", row, rowIndex, SetRowsOperation))
                return;
        }
    }

    const int oldRowCount = mRowCount;
    const int oldColumnCount = mColumnCount;

    beginResetModel();

    // We don't clear the column or role data, because a TableModel should not be reused in that way.
    // Once it has valid data, its columns and roles are fixed.
    mRows = rowsAsVariantList;
    mRowCount = mRows.size();

    // Gather metadata the first time rows is set.
    if (firstTimeValidRowsHaveBeenSet && !mRows.isEmpty())
        fetchColumnMetadata();

    endResetModel();

    emit rowsChanged();

    if (mRowCount != oldRowCount)
        emit rowCountChanged();
    if (mColumnCount != oldColumnCount)
        emit columnCountChanged();
}

QQmlTableModel::ColumnRoleMetadata QQmlTableModel::fetchColumnRoleData(const QString &roleNameKey,
    QQmlTableModelColumn *tableModelColumn, int columnIndex) const
{
    const QVariant firstRow = mRows.first();
    ColumnRoleMetadata roleData;

    QJSValue columnRoleGetter = tableModelColumn->getterAtRole(roleNameKey);
    if (columnRoleGetter.isUndefined()) {
        // This role is not defined, which is fine; just skip it.
        return roleData;
    }

    if (columnRoleGetter.isString()) {
        // The role is set as a string, so we assume the row is a simple object.
        if (firstRow.userType() != QMetaType::QVariantMap) {
            qmlWarning(this).quote() << "expected row for role "
                << roleNameKey << " of TableModelColumn at index "
                << columnIndex << " to be a simple object, but it's "
                << firstRow.typeName() << " instead: " << firstRow;
            return roleData;
        }
        const QVariantMap firstRowAsMap = firstRow.toMap();
        const QString rolePropertyName = columnRoleGetter.toString();
        const QVariant roleProperty = firstRowAsMap.value(rolePropertyName);

        roleData.isStringRole = true;
        roleData.name = rolePropertyName;
        roleData.type = roleProperty.userType();
        roleData.typeName = QString::fromLatin1(roleProperty.typeName());
    } else if (columnRoleGetter.isCallable()) {
        // The role is provided via a function, which means the row is complex and
        // the user needs to provide the data for it.
        const auto modelIndex = index(0, columnIndex);
        const auto args = QJSValueList() << qmlEngine(this)->toScriptValue(modelIndex);
        const QVariant cellData = columnRoleGetter.call(args).toVariant();

        // We don't know the property name since it's provided through the function.
        // roleData.name = ???
        roleData.isStringRole = false;
        roleData.type = cellData.userType();
        roleData.typeName = QString::fromLatin1(cellData.typeName());
    } else {
        // Invalid role.
        qmlWarning(this) << "TableModelColumn role for column at index "
            << columnIndex << " must be either a string or a function; actual type is: "
            << columnRoleGetter.toString();
    }

    return roleData;
}

void QQmlTableModel::fetchColumnMetadata()
{
    qCDebug(lcTableModel) << "gathering metadata for" << mColumnCount << "columns from first row:";

    static const auto supportedRoleNames = QQmlTableModelColumn::supportedRoleNames();

    // Since we support different data structures at the row level, we require that there
    // is a TableModelColumn for each column.
    // Collect and cache metadata for each column. This makes data lookup faster.
    for (int columnIndex = 0; columnIndex < mColumns.size(); ++columnIndex) {
        QQmlTableModelColumn *column = mColumns.at(columnIndex);
        qCDebug(lcTableModel).nospace() << "- column " << columnIndex << ":";

        ColumnMetadata metaData;
        const auto builtInRoleKeys = supportedRoleNames.keys();
        for (const int builtInRoleKey : builtInRoleKeys) {
            const QString builtInRoleName = supportedRoleNames.value(builtInRoleKey);
            ColumnRoleMetadata roleData = fetchColumnRoleData(builtInRoleName, column, columnIndex);
            if (roleData.type == QMetaType::UnknownType) {
                // This built-in role was not specified in this column.
                continue;
            }

            qCDebug(lcTableModel).nospace() << "  - added metadata for built-in role "
                << builtInRoleName << " at column index " << columnIndex
                << ": name=" << roleData.name << " typeName=" << roleData.typeName
                << " type=" << roleData.type;

            // This column now supports this specific built-in role.
            metaData.roles.insert(builtInRoleName, roleData);
            // Add it if it doesn't already exist.
            mRoleNames[builtInRoleKey] = builtInRoleName.toLatin1();
        }
        mColumnMetadata.insert(columnIndex, metaData);
    }
}

/*!
    \qmlmethod TableModel::appendRow(object row)

    Adds a new row to the end of the model, with the
    values (cells) in \a row.

    \code
        model.appendRow({
            checkable: true,
            amount: 1,
            fruitType: "Pear",
            fruitName: "Williams",
            fruitPrice: 1.50,
        })
    \endcode

    \sa insertRow(), setRow(), removeRow()
*/
void QQmlTableModel::appendRow(const QVariant &row)
{
    if (!validateNewRow("appendRow()", row, -1, AppendOperation))
        return;

    doInsert(mRowCount, row);
}

/*!
    \qmlmethod TableModel::clear()

    Removes all rows from the model.

    \sa removeRow()
*/
void QQmlTableModel::clear()
{
    QQmlEngine *engine = qmlEngine(this);
    Q_ASSERT(engine);
    setRows(QVariant::fromValue(engine->newArray()));
}

/*!
    \qmlmethod object TableModel::getRow(int rowIndex)

    Returns the row at \a rowIndex in the model.

    Note that this equivalent to accessing the row directly
    through the \l rows property:

    \code
    Component.onCompleted: {
        // These two lines are equivalent.
        console.log(model.getRow(0).display);
        console.log(model.rows[0].fruitName);
    }
    \endcode

    \note the returned object cannot be used to modify the contents of the
    model; use setRow() instead.

    \sa setRow(), appendRow(), insertRow(), removeRow(), moveRow()
*/
QVariant QQmlTableModel::getRow(int rowIndex)
{
    if (!validateRowIndex("getRow()", "rowIndex", rowIndex))
        return QVariant();

    return mRows.at(rowIndex);
}

/*!
    \qmlmethod TableModel::insertRow(int rowIndex, object row)

    Adds a new row to the list model at position \a rowIndex, with the
    values (cells) in \a row.

    \code
        model.insertRow(2, {
            checkable: true, checked: false,
            amount: 1,
            fruitType: "Pear",
            fruitName: "Williams",
            fruitPrice: 1.50,
        })
    \endcode

    The \a rowIndex must be to an existing item in the list, or one past
    the end of the list (equivalent to \l appendRow()).

    \sa appendRow(), setRow(), removeRow(), rowCount
*/
void QQmlTableModel::insertRow(int rowIndex, const QVariant &row)
{
    if (!validateNewRow("insertRow()", row, rowIndex))
        return;

    doInsert(rowIndex, row);
}

void QQmlTableModel::doInsert(int rowIndex, const QVariant &row)
{
    beginInsertRows(QModelIndex(), rowIndex, rowIndex);

    // Adding rowAsVariant.toList() will add each invidual variant in the list,
    // which is definitely not what we want.
    const QVariant rowAsVariant = row.value<QJSValue>().toVariant();
    mRows.insert(rowIndex, rowAsVariant);
    ++mRowCount;

    qCDebug(lcTableModel).nospace() << "inserted the following row to the model at index "
        << rowIndex << ":\n" << rowAsVariant.toMap();

    // Gather metadata the first time a row is added.
    if (mColumnMetadata.isEmpty())
        fetchColumnMetadata();

    endInsertRows();
    emit rowCountChanged();
}

void QQmlTableModel::classBegin()
{
}

void QQmlTableModel::componentComplete()
{
    componentCompleted = true;

    mColumnCount = mColumns.size();
    if (mColumnCount > 0)
        emit columnCountChanged();

    doSetRows(mRows);
}

/*!
    \qmlmethod TableModel::moveRow(int fromRowIndex, int toRowIndex, int rows)

    Moves \a rows from the index at \a fromRowIndex to the index at
    \a toRowIndex.

    The from and to ranges must exist; for example, to move the first 3 items
    to the end of the list:

    \code
        model.moveRow(0, model.rowCount - 3, 3)
    \endcode

    \sa appendRow(), insertRow(), removeRow(), rowCount
*/
void QQmlTableModel::moveRow(int fromRowIndex, int toRowIndex, int rows)
{
    if (fromRowIndex == toRowIndex) {
        qmlWarning(this) << "moveRow(): \"fromRowIndex\" cannot be equal to \"toRowIndex\"";
        return;
    }

    if (rows <= 0) {
        qmlWarning(this) << "moveRow(): \"rows\" is less than or equal to 0";
        return;
    }

    if (!validateRowIndex("moveRow()", "fromRowIndex", fromRowIndex))
        return;

    if (!validateRowIndex("moveRow()", "toRowIndex", toRowIndex))
        return;

    if (fromRowIndex + rows > mRowCount) {
        qmlWarning(this) << "moveRow(): \"fromRowIndex\" (" << fromRowIndex
            << ") + \"rows\" (" << rows << ") = " << (fromRowIndex + rows)
            << ", which is greater than rowCount() of " << mRowCount;
        return;
    }

    if (toRowIndex + rows > mRowCount) {
        qmlWarning(this) << "moveRow(): \"toRowIndex\" (" << toRowIndex
            << ") + \"rows\" (" << rows << ") = " << (toRowIndex + rows)
            << ", which is greater than rowCount() of " << mRowCount;
        return;
    }

    qCDebug(lcTableModel).nospace() << "moving " << rows
        << " row(s) from index " << fromRowIndex
        << " to index " << toRowIndex;

    // Based on the same call in QQmlListModel::moveRow().
    beginMoveRows(QModelIndex(), fromRowIndex, fromRowIndex + rows - 1, QModelIndex(),
        toRowIndex > fromRowIndex ? toRowIndex + rows : toRowIndex);

    // Based on ListModel::moveRow().
    if (fromRowIndex > toRowIndex) {
        // Only move forwards - flip if moving backwards.
        const int from = fromRowIndex;
        const int to = toRowIndex;
        fromRowIndex = to;
        toRowIndex = to + rows;
        rows = from - to;
    }

    QVector<QVariant> store;
    store.reserve(rows);
    for (int i = 0; i < (toRowIndex - fromRowIndex); ++i)
        store.append(mRows.at(fromRowIndex + rows + i));
    for (int i = 0; i < rows; ++i)
        store.append(mRows.at(fromRowIndex + i));
    for (int i = 0; i < store.size(); ++i)
        mRows[fromRowIndex + i] = store[i];

    qCDebug(lcTableModel).nospace() << "after moving, rows are:\n" << mRows;

    endMoveRows();
}

/*!
    \qmlmethod TableModel::removeRow(int rowIndex, int rows = 1)

    Removes the row at \a rowIndex from the model.

    \sa clear(), rowCount
*/
void QQmlTableModel::removeRow(int rowIndex, int rows)
{
    if (!validateRowIndex("removeRow()", "rowIndex", rowIndex))
        return;

    if (rows <= 0) {
        qmlWarning(this) << "removeRow(): \"rows\" is less than or equal to zero";
        return;
    }

    if (rowIndex + rows - 1 >= mRowCount) {
        qmlWarning(this) << "removeRow(): \"rows\" " << rows
            << " exceeds available rowCount() of " << mRowCount
            << " when removing from \"rowIndex\" " << rowIndex;
        return;
    }

    beginRemoveRows(QModelIndex(), rowIndex, rowIndex + rows - 1);

    auto firstIterator = mRows.begin() + rowIndex;
    // The "last" argument to erase() is exclusive, so we go one past the last item.
    auto lastIterator = firstIterator + rows;
    mRows.erase(firstIterator, lastIterator);
    mRowCount -= rows;

    endRemoveRows();
    emit rowCountChanged();

    qCDebug(lcTableModel).nospace() << "removed " << rows
        << " items from the model, starting at index " << rowIndex;
}

/*!
    \qmlmethod TableModel::setRow(int rowIndex, object row)

    Changes the row at \a rowIndex in the model with \a row.

    All columns/cells must be present in \c row, and in the correct order.

    \code
        model.setRow(0, {
            checkable: true,
            amount: 1,
            fruitType: "Pear",
            fruitName: "Williams",
            fruitPrice: 1.50,
        })
    \endcode

    If \a rowIndex is equal to \c rowCount(), then a new row is appended to the
    model. Otherwise, \a rowIndex must point to an existing row in the model.

    \sa appendRow(), insertRow(), rowCount
*/
void QQmlTableModel::setRow(int rowIndex, const QVariant &row)
{
    if (!validateNewRow("setRow()", row, rowIndex))
        return;

    if (rowIndex != mRowCount) {
        // Setting an existing row.
        mRows[rowIndex] = row;

        // For now we just assume the whole row changed, as it's simpler.
        const QModelIndex topLeftModelIndex(createIndex(rowIndex, 0));
        const QModelIndex bottomRightModelIndex(createIndex(rowIndex, mColumnCount - 1));
        emit dataChanged(topLeftModelIndex, bottomRightModelIndex);
    } else {
        // Appending a row.
        doInsert(rowIndex, row);
    }
}

QQmlListProperty<QQmlTableModelColumn> QQmlTableModel::columns()
{
    return QQmlListProperty<QQmlTableModelColumn>(this, nullptr,
        &QQmlTableModel::columns_append,
        &QQmlTableModel::columns_count,
        &QQmlTableModel::columns_at,
        &QQmlTableModel::columns_clear,
        &QQmlTableModel::columns_replace,
        &QQmlTableModel::columns_removeLast);
}

void QQmlTableModel::columns_append(QQmlListProperty<QQmlTableModelColumn> *property,
    QQmlTableModelColumn *value)
{
    QQmlTableModel *model = static_cast<QQmlTableModel*>(property->object);
    QQmlTableModelColumn *column = qobject_cast<QQmlTableModelColumn*>(value);
    if (column)
        model->mColumns.append(column);
}

int QQmlTableModel::columns_count(QQmlListProperty<QQmlTableModelColumn> *property)
{
    const QQmlTableModel *model = static_cast<QQmlTableModel*>(property->object);
    return model->mColumns.count();
}

QQmlTableModelColumn *QQmlTableModel::columns_at(QQmlListProperty<QQmlTableModelColumn> *property, int index)
{
    const QQmlTableModel *model = static_cast<QQmlTableModel*>(property->object);
    return model->mColumns.at(index);
}

void QQmlTableModel::columns_clear(QQmlListProperty<QQmlTableModelColumn> *property)
{
    QQmlTableModel *model = static_cast<QQmlTableModel*>(property->object);
    return model->mColumns.clear();
}

void QQmlTableModel::columns_replace(QQmlListProperty<QQmlTableModelColumn> *property, int index, QQmlTableModelColumn *value)
{
    QQmlTableModel *model = static_cast<QQmlTableModel*>(property->object);
    if (QQmlTableModelColumn *column = qobject_cast<QQmlTableModelColumn*>(value))
        return model->mColumns.replace(index, column);
}

void QQmlTableModel::columns_removeLast(QQmlListProperty<QQmlTableModelColumn> *property)
{
    QQmlTableModel *model = static_cast<QQmlTableModel*>(property->object);
    model->mColumns.removeLast();
}

/*!
    \qmlmethod QModelIndex TableModel::index(int row, int column)

    Returns a \l QModelIndex object referencing the given \a row and \a column,
    which can be passed to the data() function to get the data from that cell,
    or to setData() to edit the contents of that cell.

    \code
    import QtQml 2.14
    import Qt.labs.qmlmodels 1.0

    TableModel {
        id: model

        TableModelColumn { display: "fruitType" }
        TableModelColumn { display: "fruitPrice" }

        rows: [
            { fruitType: "Apple", fruitPrice: 1.50 },
            { fruitType: "Orange", fruitPrice: 2.50 }
        ]

        Component.onCompleted: {
            for (var r = 0; r < model.rowCount; ++r) {
                console.log("An " + model.data(model.index(r, 0)).display +
                            " costs " + model.data(model.index(r, 1)).display.toFixed(2))
            }
        }
    }
    \endcode

    \sa {QModelIndex and related Classes in QML}, data()
*/
// Note: we don't document the parent argument, because you never need it, because
// cells in a TableModel don't have parents.  But it is there because this function is an override.
QModelIndex QQmlTableModel::index(int row, int column, const QModelIndex &parent) const
{
    return row >= 0 && row < rowCount() && column >= 0 && column < columnCount() && !parent.isValid()
        ? createIndex(row, column)
        : QModelIndex();
}

/*!
    \qmlproperty int TableModel::rowCount
    \readonly

    This read-only property holds the number of rows in the model.

    This value changes whenever rows are added or removed from the model.
*/
int QQmlTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return mRowCount;
}

/*!
    \qmlproperty int TableModel::columnCount
    \readonly

    This read-only property holds the number of columns in the model.

    The number of columns is fixed for the lifetime of the model
    after the \l rows property is set or \l appendRow() is called for the first
    time.
*/
int QQmlTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return mColumnCount;
}

/*!
    \qmlmethod variant TableModel::data(QModelIndex index, string role)

    Returns the data from the table cell at the given \a index belonging to the
    given \a role.

    \sa index()
*/
QVariant QQmlTableModel::data(const QModelIndex &index, const QString &role) const
{
    const int iRole = mRoleNames.key(role.toUtf8(), -1);
    if (iRole >= 0)
        return data(index, iRole);
    return QVariant();
}

QVariant QQmlTableModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < 0 || row >= rowCount())
        return QVariant();

    const int column = index.column();
    if (column < 0 || column >= columnCount())
        return QVariant();

    const ColumnMetadata columnMetadata = mColumnMetadata.at(index.column());
    const QString roleName = QString::fromUtf8(mRoleNames.value(role));
    if (!columnMetadata.roles.contains(roleName)) {
        qmlWarning(this) << "setData(): no role named " << roleName
            << " at column index " << column << ". The available roles for that column are: "
            << columnMetadata.roles.keys();
        return QVariant();
    }

    const ColumnRoleMetadata roleData = columnMetadata.roles.value(roleName);
    if (roleData.isStringRole) {
        // We know the data structure, so we can get the data for the user.
        const QVariantMap rowData = mRows.at(row).toMap();
        const QString propertyName = columnMetadata.roles.value(roleName).name;
        const QVariant value = rowData.value(propertyName);
        return value;
    }

    // We don't know the data structure, so the user has to modify their data themselves.
    // First, find the getter for this column and role.
    QJSValue getter = mColumns.at(column)->getterAtRole(roleName);

    // Then, call it and return what it returned.
    const auto args = QJSValueList() << qmlEngine(this)->toScriptValue(index);
    return getter.call(args).toVariant();
}

/*!
    \qmlmethod bool TableModel::setData(QModelIndex index, string role, variant value)

    Inserts or updates the data field named by \a role in the table cell at the
    given \a index with \a value. Returns true if sucessful, false if not.

    \sa index()
*/
bool QQmlTableModel::setData(const QModelIndex &index, const QString &role, const QVariant &value)
{
    const int intRole = mRoleNames.key(role.toUtf8(), -1);
    if (intRole >= 0)
        return setData(index, value, intRole);
    return false;
}

bool QQmlTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    const int row = index.row();
    if (row < 0 || row >= rowCount())
        return false;

    const int column = index.column();
    if (column < 0 || column >= columnCount())
        return false;

    const QString roleName = QString::fromUtf8(mRoleNames.value(role));

    qCDebug(lcTableModel).nospace() << "setData() called with index "
        << index << ", value " << value << " and role " << roleName;

    // Verify that the role exists for this column.
    const ColumnMetadata columnMetadata = mColumnMetadata.at(index.column());
    if (!columnMetadata.roles.contains(roleName)) {
        qmlWarning(this) << "setData(): no role named \"" << roleName
            << "\" at column index " << column << ". The available roles for that column are: "
            << columnMetadata.roles.keys();
        return false;
    }

    // Verify that the type of the value is what we expect.
    // If the value set is not of the expected type, we can try to convert it automatically.
    const ColumnRoleMetadata roleData = columnMetadata.roles.value(roleName);
    QVariant effectiveValue = value;
    if (value.userType() != roleData.type) {
        if (!value.canConvert(int(roleData.type))) {
            qmlWarning(this).nospace() << "setData(): the value " << value
                << " set at row " << row << " column " << column << " with role " << roleName
                << " cannot be converted to " << roleData.typeName;
            return false;
        }

        if (!effectiveValue.convert(int(roleData.type))) {
            qmlWarning(this).nospace() << "setData(): failed converting value " << value
                << " set at row " << row << " column " << column << " with role " << roleName
                << " to " << roleData.typeName;
            return false;
        }
    }

    if (roleData.isStringRole) {
        // We know the data structure, so we can set it for the user.
        QVariantMap modifiedRow = mRows.at(row).toMap();
        modifiedRow[roleData.name] = value;

        mRows[row] = modifiedRow;
    } else {
        // We don't know the data structure, so the user has to modify their data themselves.
        auto engine = qmlEngine(this);
        auto args = QJSValueList()
            // arg 0: modelIndex.
            << engine->toScriptValue(index)
            // arg 1: cellData.
            << engine->toScriptValue(value);
        // Do the actual setting.
        QJSValue setter = mColumns.at(column)->setterAtRole(roleName);
        setter.call(args);

        /*
            The chain of events so far:

            - User did e.g.: model.edit = textInput.text
              - setData() is called
                - setData() calls the setter
                  (remember that we need to emit the dataChanged() signal,
                   which is why the user can't just set the data directly in the delegate)

            Now the user's setter function has modified *their* copy of the
            data, but *our* copy of the data is old. Imagine the getters and setters looked like this:

            display: function(modelIndex) { return rows[modelIndex.row][1].amount }
            setDisplay: function(modelIndex, cellData) { rows[modelIndex.row][1].amount = cellData }

            We don't know the structure of the user's data, so we can't just do
            what we do above for the isStringRole case:

            modifiedRow[column][roleName] = value

            This means that, besides getting the implicit row count when rows is initially set,
            our copy of the data is unused when it comes to complex columns.

            Another point to note is that we can't pass rowData in to the getter as a convenience,
            because we would be passing in *our* copy of the row, which is not up-to-date.
            Since the user already has access to the data, it's not a big deal for them to do:

            display: function(modelIndex) { return rows[modelIndex.row][1].amount }

            instead of:

            display: function(modelIndex, rowData) { return rowData[1].amount }
        */
    }

    QVector<int> rolesChanged;
    rolesChanged.append(role);
    emit dataChanged(index, index, rolesChanged);

    return true;
}

QHash<int, QByteArray> QQmlTableModel::roleNames() const
{
    return mRoleNames;
}

QQmlTableModel::ColumnRoleMetadata::ColumnRoleMetadata()
{
}

QQmlTableModel::ColumnRoleMetadata::ColumnRoleMetadata(
    bool isStringRole, const QString &name, int type, const QString &typeName) :
    isStringRole(isStringRole),
    name(name),
    type(type),
    typeName(typeName)
{
}

bool QQmlTableModel::ColumnRoleMetadata::isValid() const
{
    return !name.isEmpty();
}

bool QQmlTableModel::validateRowType(const char *functionName, const QVariant &row) const
{
    if (!row.canConvert<QJSValue>()) {
        qmlWarning(this) << functionName << ": expected \"row\" argument to be a QJSValue,"
            << " but got " << row.typeName() << " instead:\n" << row;
        return false;
    }

    const QJSValue rowAsJSValue = row.value<QJSValue>();
    if (!rowAsJSValue.isObject() && !rowAsJSValue.isArray()) {
        qmlWarning(this) << functionName << ": expected \"row\" argument "
            << "to be an object or array, but got:\n" << rowAsJSValue.toString();
        return false;
    }

    return true;
}

bool QQmlTableModel::validateNewRow(const char *functionName, const QVariant &row,
    int rowIndex, NewRowOperationFlag operation) const
{
    if (mColumnMetadata.isEmpty()) {
        // There is no column metadata, so we have nothing to validate the row against.
        // Rows have to be added before we can gather metadata from them, so just this
        // once we'll return true to allow the rows to be added.
        return true;
    }

    // Don't require each row to be a QJSValue when setting all rows,
    // as they won't be; they'll be QVariantMap.
    if (operation != SetRowsOperation && !validateRowType(functionName, row))
        return false;

    if (operation == OtherOperation) {
        // Inserting/setting.
        if (rowIndex < 0) {
            qmlWarning(this) << functionName << ": \"rowIndex\" cannot be negative";
            return false;
        }

        if (rowIndex > mRowCount) {
            qmlWarning(this) << functionName << ": \"rowIndex\" " << rowIndex
                << " is greater than rowCount() of " << mRowCount;
            return false;
        }
    }

    const QVariant rowAsVariant = operation == SetRowsOperation
        ? row : row.value<QJSValue>().toVariant();
    if (rowAsVariant.userType() != QMetaType::QVariantMap) {
        qmlWarning(this) << functionName << ": row manipulation functions "
            << "do not support complex rows (row index: " << rowIndex << ")";
        return false;
    }

    const QVariantMap rowAsMap = rowAsVariant.toMap();
    const int columnCount = rowAsMap.size();
    if (columnCount < mColumnCount) {
        qmlWarning(this) << functionName << ": expected " << mColumnCount
            << " columns, but only got " << columnCount;
        return false;
    }

    // We can't validate complex structures, but we can make sure that
    // each simple string-based role in each column is correct.
    for (int columnIndex = 0; columnIndex < mColumns.size(); ++columnIndex) {
        QQmlTableModelColumn *column = mColumns.at(columnIndex);
        const QHash<QString, QJSValue> getters = column->getters();
        const auto roleNames = getters.keys();
        const ColumnMetadata columnMetadata = mColumnMetadata.at(columnIndex);
        for (const QString &roleName : roleNames) {
            const ColumnRoleMetadata roleData = columnMetadata.roles.value(roleName);
            if (!roleData.isStringRole)
                continue;

            if (!rowAsMap.contains(roleData.name)) {
                qmlWarning(this).quote() << functionName << ": expected a property named "
                    << roleData.name << " in row at index " << rowIndex << ", but couldn't find one";
                return false;
            }

            const QVariant rolePropertyValue = rowAsMap.value(roleData.name);

            if (rolePropertyValue.userType() != roleData.type) {
                if (!rolePropertyValue.canConvert(int(roleData.type))) {
                    qmlWarning(this).quote() << functionName << ": expected the property named "
                        << roleData.name << " to be of type " << roleData.typeName
                        << ", but got " << QString::fromLatin1(rolePropertyValue.typeName())
                        << " instead";
                    return false;
                }

                QVariant effectiveValue = rolePropertyValue;
                if (!effectiveValue.convert(int(roleData.type))) {
                    qmlWarning(this).nospace() << functionName << ": failed converting value "
                        << rolePropertyValue << " set at column " << columnIndex << " with role "
                        << QString::fromLatin1(rolePropertyValue.typeName()) << " to "
                        << roleData.typeName;
                    return false;
                }
            }
        }
    }

    return true;
}

bool QQmlTableModel::validateRowIndex(const char *functionName, const char *argumentName, int rowIndex) const
{
    if (rowIndex < 0) {
        qmlWarning(this) << functionName << ": \"" << argumentName << "\" cannot be negative";
        return false;
    }

    if (rowIndex >= mRowCount) {
        qmlWarning(this) << functionName << ": \"" << argumentName
            << "\" " << rowIndex << " is greater than or equal to rowCount() of " << mRowCount;
        return false;
    }

    return true;
}

QT_END_NAMESPACE
