/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquicktreeview_p_p.h"

#include <QtCore/qobject.h>
#include <QtQml/qqmlcontext.h>

#include <QtQmlModels/private/qqmltreemodeltotablemodel_p_p.h>

/*!
    \qmltype TreeView
    \inqmlmodule QtQuick
    \ingroup qtquick-views
    \since 6.3
    \inherits TableView
    \brief Provides a tree view to display data from a QAbstractItemModel.

    A TreeView has a \l model that defines the data to be displayed, and a
    \l delegate that defines how the data should be displayed.

    TreeView inherits \l TableView. This means that even if the model
    has a parent-child tree structure, TreeView is internally using a
    proxy model that converts that structure into a flat table
    model that can be rendered by TableView. Each node in the tree ends up
    occupying one row in the table, where the first column renders the tree
    itself. By indenting each delegate item in that column according to its
    parent-child depth in the model, it will end up looking like a tree, even
    if it's technically still just a flat list of items.

    To allow for maximum flexibility, TreeView itself will not position the delegate
    items into a tree structure. This burden is placed on the delegate.
    \l {Qt Quick Controls} offers a ready-made TreeViewDelegate that can be
    used for this, which has the advantage that it works out-of-the-box and
    renders a tree which follows the style of the platform where the application runs.

    Even if TreeViewDelegate is customizable, there might be situations
    where you want to render the tree in a different way, or ensure that
    the delegate ends up as minimal as possible, perhaps for performance reasons.
    Creating your own delegate from scratch is easy, since TreeView offers
    a set of properties that can be used to position and render each node
    in the tree correctly.

    An example of a custom delegate is shown below:

    \snippet qml/treeview/qml-customdelegate.qml 0

    The properties that are marked as \c required will be filled in by
    TreeView, and are similar to attached properties. By marking them as
    required, the delegate indirectly informs TreeView that it should take
    responsibility for assigning them values. The following required properties
    can be added to a delegate:

    \list
        \li \c {required property TreeView treeView}
                - Points to the TreeView that contains the delegate item.
        \li \c {required property bool isTreeNode}
                - Is \c true if the delegate item represents a node in
                the tree. Only one column in the view will be used to draw the tree, and
                therefore, only delegate items in that column will have this
                property set to \c true.
                A node in the tree should typically be indented according to its
                \c depth, and show an indicator if \c hasChildren is \c true.
                Delegate items in other columns will have this property set to
                \c false, and will show data from the remaining columns
                in the model (and typically not be indented).
        \li \c {required property bool expanded}
                - Is \c true if the model item drawn by the delegate is expanded
                in the view.
        \li \c {required property int hasChildren}
                - Is \c true if the model item drawn by the delegate has children
                in the model.
        \li \c {required property int depth}
                - Contains the depth of the model item drawn by the delegate.
                The depth of a model item is the same as the number of ancestors
                it has in the model.
    \endlist

    See also \l {Required Properties}.

    \note A TreeView only accepts a model that inherits \l QAbstractItemModel.
*/

/*!
    \qmlmethod int QtQuick::TreeView::depth(row)

    Returns the depth (the number of parents up to the root) of the given \a row.

    \a row should be the row in the view (table row), and not a row in the model.
    If \a row is not between \c 0 and \l {TableView::}{rows}, the return value will
    be \c -1.

    \sa modelIndex()
*/

/*!
    \qmlmethod bool QtQuick::TreeView::isExpanded(row)

    Returns if the given \a row in the view is shown as expanded.

    \a row should be the row in the view (table row), and not a row in the model.
    If \a row is not between \c 0 and \l {TableView::}{rows}, the return value will
    be \c false.
*/

/*!
    \qmlmethod QtQuick::TreeView::expand(row)

    Expands the tree node at the given \a row in the view.

    \a row should be the row in the view (table row), and not a row in the model.

    \note this function will not affect the model, only
    the visual representation in the view.

    \sa collapse(), isExpanded()
*/

/*!
    \qmlmethod QtQuick::TreeView::collapse(row)

    Collapses the tree node at the given \a row in the view.

    \a row should be the row in the view (table row), and not a row in the model.

    \note this function will not affect the model, only
    the visual representation in the view.

    \sa expand(), isExpanded()
*/

/*!
    \qmlmethod QtQuick::TreeView::toggleExpanded(row)

    Toggles if the tree node at the given \a row should be expanded.
    This is a convenience for doing:

    \code
    if (isExpanded(row))
        collapse(row)
    else
        expand(row)
    \endcode

    \a row should be the row in the view (table row), and not a row in the model.
*/

/*!
    \qmlmethod QModelIndex QtQuick::TreeView::modelIndex(row, column)

    Returns the \l QModelIndex that maps to \a row and \a column in the view.

    \a row and \a column should be the row and column in the view (table row and
    table column), and not a row and column in the model.

    The assigned model, which is a tree model, is converted to a flat table
    model internally so that it can be shown in a TableView (which TreeView
    inherits). This function can be used whenever you need to know which
    index in the tree model maps to the given row and column in the view.

    \sa rowAtIndex(), columnAtIndex()
*/

/*!
    \qmlmethod QModelIndex QtQuick::TreeView::modelIndex(point cell)

    Convenience function for doing:
    \code
    modelIndex(cell.y, cell.x)
    \endcode

    A cell is simply a \l point that combines row and column into
    a single type. Note that \c point.x will map to the column, and
    \c point.y will map to the row.
*/

/*!
    \qmlmethod int QtQuick::TreeView::rowAtIndex(modelIndex)

    Returns the row in the view that maps to \a modelIndex in the model.

    The assigned model, which is a tree model, is converted to a flat table
    model internally so that it can be shown in a TableView (which TreeView
    inherits). This function can be used whenever you need to know which
    row in the view the given model index maps to.

    \note \a modelIndex must be a \l QModelIndex.

    \sa columnAtIndex(), modelIndex()
*/

/*!
    \qmlmethod int QtQuick::TreeView::columnAtIndex(modelIndex)

    Returns the column in the view that maps to \a modelIndex in the model.

    The assigned model, which is a tree model, is converted to a flat table
    model internally so that it can be shown in a TableView (which TreeView
    inherits). This function can be used whenever you need to know which
    column in the view the given model index maps to.

    \note \a modelIndex must be a \l QModelIndex.

    \sa rowAtIndex(), modelIndex()
*/

/*!
    \qmlmethod point QtQuick::TreeView::cellAtIndex(modelIndex)

    Convenience function for doing:

    \c {Qt.point(columnAtIndex(}\a {modelIndex}\c{), rowAtIndex(}\a {modelIndex}\c{))}

    A cell is simply a \l point that combines row and column into
    a single type. Note that \c point.x will map to the column, and
    \c point.y will map to the row.
*/

/*!
    \qmlsignal QtQuick::TreeView::expanded(row)

    This signal is emitted when a \a row is expanded in the view.

    \sa collapsed(), expand(), collapse(), toggleExpanded()
*/

/*!
    \qmlsignal QtQuick::TreeView::collapsed(row)

    This signal is emitted when a \a row is collapsed in the view.

    \sa expanded(), expand(), collapse(), toggleExpanded()
*/

static const char* kRequiredProperties = "_qt_treeview_requiredpropertymask";
// Hard-code the tree column to be 0 for now
static const int kTreeColumn = 0;

QT_BEGIN_NAMESPACE

QQuickTreeViewPrivate::QQuickTreeViewPrivate()
    : QQuickTableViewPrivate()
{
}

QQuickTreeViewPrivate::~QQuickTreeViewPrivate()
{
}

QVariant QQuickTreeViewPrivate::modelImpl() const
{
    return m_assignedModel;
}

void QQuickTreeViewPrivate::setModelImpl(const QVariant &newModel)
{
    Q_Q(QQuickTreeView);

    if (newModel == m_assignedModel)
        return;

    m_assignedModel = newModel;
    QVariant effectiveModel = m_assignedModel;
    if (effectiveModel.userType() == qMetaTypeId<QJSValue>())
        effectiveModel = effectiveModel.value<QJSValue>().toVariant();

    if (effectiveModel.isNull())
        m_treeModelToTableModel.setModel(nullptr);
    else if (const auto qaim = qvariant_cast<QAbstractItemModel*>(effectiveModel))
        m_treeModelToTableModel.setModel(qaim);
    else
        qmlWarning(q) << "TreeView only accepts a model of type QAbstractItemModel";


    scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::All);
    emit q->modelChanged();
}

void QQuickTreeViewPrivate::initItemCallback(int serializedModelIndex, QObject *object)
{
    updateRequiredProperties(serializedModelIndex, object, true);
    QQuickTableViewPrivate::initItemCallback(serializedModelIndex, object);
}

void QQuickTreeViewPrivate::itemReusedCallback(int serializedModelIndex, QObject *object)
{
    updateRequiredProperties(serializedModelIndex, object, false);
    QQuickTableViewPrivate::itemReusedCallback(serializedModelIndex, object);
}

void QQuickTreeViewPrivate::dataChangedCallback(
        const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_Q(QQuickTreeView);
    Q_UNUSED(roles);

    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
        for (int column = topLeft.column(); column <= bottomRight.column(); ++column) {
            const QPoint cell(column, row);
            auto item = q->itemAtCell(cell);
            if (!item)
                continue;

            const int serializedModelIndex = modelIndexAtCell(QPoint(column, row));
            updateRequiredProperties(serializedModelIndex, item, false);
        }
    }
}

void QQuickTreeViewPrivate::setRequiredProperty(const char *property,
    const QVariant &value, int serializedModelIndex, QObject *object, bool init)
{
    // Attaching a property list to the delegate item is just a
    // work-around until QMetaProperty::isRequired() works!
    const QString propertyName = QString::fromUtf8(property);

    if (init) {
        const bool wasRequired = model->setRequiredProperty(serializedModelIndex, propertyName, value);
        if (wasRequired) {
            QStringList propertyList = object->property(kRequiredProperties).toStringList();
            object->setProperty(kRequiredProperties, propertyList << propertyName);
        }
    } else {
        const QStringList propertyList = object->property(kRequiredProperties).toStringList();
        if (!propertyList.contains(propertyName)) {
            // We only write to properties that are required
            return;
        }
        const auto metaObject = object->metaObject();
        const int propertyIndex = metaObject->indexOfProperty(property);
        const auto metaProperty = metaObject->property(propertyIndex);
        metaProperty.write(object, value);
    }
}

void QQuickTreeViewPrivate::updateRequiredProperties(int serializedModelIndex, QObject *object, bool init)
{
    Q_Q(QQuickTreeView);
    const QPoint cell = cellAtModelIndex(serializedModelIndex);
    const int row = cell.y();
    const int column = cell.x();

    setRequiredProperty("treeView", QVariant::fromValue(q), serializedModelIndex, object, init);
    setRequiredProperty("isTreeNode", column == kTreeColumn, serializedModelIndex, object, init);
    setRequiredProperty("hasChildren", m_treeModelToTableModel.hasChildren(row), serializedModelIndex, object, init);
    setRequiredProperty("expanded", q->isExpanded(row), serializedModelIndex, object, init);
    setRequiredProperty("depth", m_treeModelToTableModel.depthAtRow(row), serializedModelIndex, object, init);
}

QQuickTreeView::QQuickTreeView(QQuickItem *parent)
    : QQuickTableView(*(new QQuickTreeViewPrivate), parent)
{
    Q_D(QQuickTreeView);

    // Note: QQuickTableView will only ever see the table model m_treeModelToTableModel, and
    // never the actual tree model that is assigned to us by the application.
    const auto modelAsVariant = QVariant::fromValue(std::addressof(d->m_treeModelToTableModel));
    d->QQuickTableViewPrivate::setModelImpl(modelAsVariant);
    QObjectPrivate::connect(&d->m_treeModelToTableModel, &QAbstractItemModel::dataChanged,
                            d, &QQuickTreeViewPrivate::dataChangedCallback);
}

QQuickTreeView::~QQuickTreeView()
{
}

int QQuickTreeView::depth(int row) const
{
    Q_D(const QQuickTreeView);
    if (row < 0 || row >= d->m_treeModelToTableModel.rowCount())
        return -1;

    return d->m_treeModelToTableModel.depthAtRow(row);
}

bool QQuickTreeView::isExpanded(int row) const
{
    Q_D(const QQuickTreeView);
    if (row < 0 || row >= d->m_treeModelToTableModel.rowCount())
        return false;

    return d->m_treeModelToTableModel.isExpanded(row);
}

void QQuickTreeView::expand(int row)
{
    Q_D(QQuickTreeView);
    if (row < 0 || row >= d->m_treeModelToTableModel.rowCount())
        return;

    if (d->m_treeModelToTableModel.isExpanded(row))
        return;

    d->m_treeModelToTableModel.expandRow(row);

    for (int c = leftColumn(); c <= rightColumn(); ++c) {
        const QPoint treeNodeCell(c, row);
        if (const auto item = itemAtCell(treeNodeCell))
            d->setRequiredProperty("expanded", true, d->modelIndexAtCell(treeNodeCell), item, false);
    }

    emit expanded(row);
}

void QQuickTreeView::collapse(int row)
{
    Q_D(QQuickTreeView);
    if (row < 0 || row >= d->m_treeModelToTableModel.rowCount())
        return;

    if (!d->m_treeModelToTableModel.isExpanded(row))
        return;

    d_func()->m_treeModelToTableModel.collapseRow(row);

    for (int c = leftColumn(); c <= rightColumn(); ++c) {
        const QPoint treeNodeCell(c, row);
        if (const auto item = itemAtCell(treeNodeCell))
            d->setRequiredProperty("expanded", false, d->modelIndexAtCell(treeNodeCell), item, false);
    }

    emit collapsed(row);
}

void QQuickTreeView::toggleExpanded(int row)
{
    if (isExpanded(row))
        collapse(row);
    else
        expand(row);
}

QModelIndex QQuickTreeView::modelIndex(int row, int column) const
{
    Q_D(const QQuickTreeView);
    const QModelIndex tableIndex = d->m_treeModelToTableModel.index(row, column);
    return d->m_treeModelToTableModel.mapToModel(tableIndex);
}

QModelIndex QQuickTreeView::modelIndex(const QPoint &cell) const
{
    return modelIndex(cell.y(), cell.x());
}

int QQuickTreeView::rowAtIndex(const QModelIndex &index) const
{
    return d_func()->m_treeModelToTableModel.mapFromModel(index).row();
}

int QQuickTreeView::columnAtIndex(const QModelIndex &index) const
{
    return d_func()->m_treeModelToTableModel.mapFromModel(index).column();
}

QPoint QQuickTreeView::cellAtIndex(const QModelIndex &index) const
{
    const QModelIndex tableIndex = d_func()->m_treeModelToTableModel.mapFromModel(index);
    return QPoint(tableIndex.column(), tableIndex.row());
}

QT_END_NAMESPACE

#include "moc_qquicktreeview_p.cpp"
