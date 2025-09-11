// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmltablemodel_p.h"

#include <QtCore/qloggingcategory.h>

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(lcTableModel, "qt.qml.tablemodel")

/*!
    \qmltype TableModel
//!    \nativetype QQmlTableModel
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
    is a list of simple key-value pairs:

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
    with the source data that was set in QML. For these reasons, manipulation
    of the data is not supported.

    For example, suppose you wanted to use a data source where each row is an
    array and each cell is an object. To use this data source with TableModel,
    define a getter:

    \code
    TableModel {
        TableModelColumn {
            display: function(modelIndex) { return rows[modelIndex.row][0].checked }
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
    : QQmlAbstractColumnModel(parent)
{
}

QQmlTableModel::~QQmlTableModel()
    = default;

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

    const auto rowsAsJSValue = rows.value<QJSValue>();
    const QVariantList rowsAsVariantList = rowsAsJSValue.toVariant().toList();
    if (rowsAsVariantList == mRows) {
        // No change.
        return;
    }

    if (!mComponentCompleted) {
        // Store the rows until we can call setRowsPrivate() after component completion.
        mRows = rowsAsVariantList;
        return;
    }

    setRowsPrivate(rowsAsVariantList);
}

void QQmlTableModel::setRowsPrivate(const QVariantList &rowsAsVariantList)
{
    Q_ASSERT(mComponentCompleted);

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
            if (!validateNewRow("setRows()"_L1, row, SetRowsOperation))
                return;
        }
    }

    const int oldRowCount = mRowCount;

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
}

QVariant QQmlTableModel::dataPrivate(const QModelIndex &index, const QString &roleName) const
{
    const ColumnMetadata columnMetadata = mColumnMetadata.at(index.column());
    const QString propertyName = columnMetadata.roles.value(roleName).name;
    const QVariantMap rowData = mRows.at(index.row()).toMap();
    return rowData.value(propertyName);
}

void QQmlTableModel::setDataPrivate(const QModelIndex &index, const QString &roleName, QVariant value)
{
    int row = index.row();
    QVariantMap modifiedRow = mRows.at(row).toMap();
    modifiedRow[roleName] = value;
    mRows[row] = modifiedRow;
}

// TODO: Turn this into a snippet that compiles in CI
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
    if (!validateNewRow("appendRow()"_L1, row, AppendOperation))
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
    if (!validateRowIndex("getRow()"_L1, "rowIndex"_L1, rowIndex, NeedsExisting))
        return QVariant();
    return mRows.at(rowIndex);
}

/*!
    \qmlmethod TableModel::insertRow(int rowIndex, object row)

    Adds a new row to the model at position \a rowIndex, with the
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

    The \a rowIndex must point to an existing item in the table, or one past
    the end of the table (equivalent to \l appendRow()).

    \sa appendRow(), setRow(), removeRow(), rowCount
*/
void QQmlTableModel::insertRow(int rowIndex, const QVariant &row)
{
    if (!validateNewRow("insertRow()"_L1, row) ||
        !validateRowIndex("insertRow()"_L1, "rowIndex"_L1, rowIndex, CanAppend))
        return;

    doInsert(rowIndex, row);
}

void QQmlTableModel::doInsert(int rowIndex, const QVariant &row)
{
    beginInsertRows(QModelIndex(), rowIndex, rowIndex);

    // Adding rowAsVariant.toList() will add each invidual variant in the list,
    // which is definitely not what we want.
    const QVariant rowAsVariant = row.userType() == QMetaType::QVariantMap ? row : row.value<QJSValue>().toVariant();

    mRows.insert(rowIndex, rowAsVariant);
    ++mRowCount;

    qCDebug(lcTableModel).nospace() << "inserted the following row to the model at index "
        << rowIndex << ":\n" << rowAsVariant.toMap();

    // Gather metadata the first time a row is added.
    if (mColumnMetadata.isEmpty())
        fetchColumnMetadata();

    endInsertRows();
    emit rowCountChanged();
    emit rowsChanged();
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

    if (!validateRowIndex("moveRow()"_L1, "fromRowIndex"_L1, fromRowIndex, NeedsExisting))
        return;

    if (!validateRowIndex("moveRow()"_L1, "toRowIndex"_L1, toRowIndex, NeedsExisting))
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
    emit rowsChanged();
}

/*!
    \qmlmethod TableModel::removeRow(int rowIndex, int rows = 1)

    Removes a number of \a rows at \a rowIndex from the model.

    \sa clear(), rowCount
*/
void QQmlTableModel::removeRow(int rowIndex, int rows)
{
    if (!validateRowIndex("removeRow()"_L1, "rowIndex"_L1, rowIndex, NeedsExisting))
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
    emit rowsChanged();

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
    if (!validateNewRow("setRow()"_L1, row) ||
        !validateRowIndex("setRow()"_L1, "rowIndex"_L1, rowIndex, CanAppend))
        return;

    if (rowIndex != mRowCount) {
        // Setting an existing row.
        mRows[rowIndex] = row;

        // For now we just assume the whole row changed, as it's simpler.
        const QModelIndex topLeftModelIndex(createIndex(rowIndex, 0));
        const QModelIndex bottomRightModelIndex(createIndex(rowIndex, mColumnCount - 1));
        emit dataChanged(topLeftModelIndex, bottomRightModelIndex);
        emit rowsChanged();
    } else {
        // Appending a row.
        doInsert(rowIndex, row);
    }
}


QVariant QQmlTableModel::firstRow() const
{
    return mRows.first();
}

void QQmlTableModel::setInitialRows()
{
    setRowsPrivate(mRows);
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

QModelIndex QQmlTableModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return {};
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
    Q_UNUSED(parent);

    return mColumnCount;
}

/*!
    \qmlmethod variant TableModel::data(QModelIndex index, string role)

    Returns the data from the table cell at the given \a index belonging to the
    given \a role.

    \sa index(), setData()
*/

/*!
    \qmlmethod bool TableModel::setData(QModelIndex index, variant value, string role)

    Inserts or updates the data field named by \a role in the table cell at the
    given \a index with \a value. Returns true if sucessful, false if not.

    \sa data(), index()
*/

bool QQmlTableModel::validateRowIndex(QLatin1StringView functionName, QLatin1StringView argumentName,
                                      int rowIndex, RowOption operation) const
{
    if (rowIndex < 0) {
        qmlWarning(this).noquote() << functionName << ": \"" << argumentName << "\" cannot be negative";
        return false;
    }

    if (operation == NeedsExisting) {
        if (rowIndex >= mRowCount) {
            qmlWarning(this).noquote() << functionName << ": \"" << argumentName
                << "\" " << rowIndex << " is greater than or equal to rowCount() of " << mRowCount;
            return false;
        }
    } else {
        if (rowIndex > mRowCount) {
            qmlWarning(this).noquote() << functionName << ": \"" << argumentName
                << "\" " << rowIndex << " is greater than rowCount() of " << mRowCount;
            return false;
        }
    }

    return true;
}

QT_END_NAMESPACE

#include "moc_qqmltablemodel_p.cpp"
