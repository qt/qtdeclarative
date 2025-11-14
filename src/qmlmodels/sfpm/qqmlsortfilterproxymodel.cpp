// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qitemselectionmodel.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>
#include <QtQmlModels/private/qqmlfiltercompositor_p.h>
#include <QtQmlModels/private/qqmlsortercompositor_p.h>
#include <QtQmlModels/private/qqmlrolefilter_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY (lcSortFilterProxyModel, "qt.qml.sortfilterproxymodel")

/*!
    \qmltype SortFilterProxyModel
    //! \nativetype QQmlSortFilterProxyModel
    \inqmlmodule QtQml.Models
    \since 6.10
    \preliminary
    \brief Provides sorting and filtering capabilities for a
    \l QAbstractItemModel.

    SortFilterProxyModel inherits from \l QAbstractProxyModel, which handles
    the transformation of source indexes into proxy indexes. Sorting and
    filtering are controlled via the \l{SortFilterProxyModel::sorters}{sorters}
    and \l{SortFilterProxyModel::filters}{filters} properties, which
    determine the order of execution as specified in QML. This order can be
    overridden using the priority property available for each sorter. By
    default, all sorters share the same priority.

    SortFilterProxyModel enables users to sort and filter a
    \l QAbstractItemModel. The item views such as \l TableView or \l TreeView
    can utilize this proxy model to display filtered content.

    \note Currently, SortFilterProxyModel supports only QAbstractItemModel
    as the source model.

    The snippet below shows the configuration of sorters and filters in the
    QML SortFilterProxyModel:

    \qml
    ProcessModel { id: processModel }

    SortFilterProxyModel {
        id: sfpm
        model: processModel
        sorters: [
            RoleSorter: {
                roleName: "user"
                priority: 0
            },
            RoleSorter: {
                roleName: "pid"
                priority: 1
            }
        ]
        filters: [
            FunctionFilter: {
                component RoleData: QtObject { property qreal cpuUsage }
                function filter(data: RoleData) : bool {
                    return (data.cpuUsage > 90)
                }
            }
        ]
    }
    \endqml

    The SortFilterProxyModel dynamically sorts and filters data whenever there
    is a change to the data in the source model and can be disabled through the
    \l{SortFilterProxyModel::}{dynamicSortFilter} property.

    The sorters \l RoleSorter, \l StringSorter and \l FunctionSorter can be
    configured in SortFilterProxyModel. Each sorter can be configured with a
    specific column index through \l{Sorter::}{column} property. If a
    column index is not specified, the sorting will be applied to the column
    index 0 of the model by default. The execution order of the sorter can be
    modified through the \l{Sorter::priority}{priority} property. This is
    particularly useful when performing hierarchical sorting, such as sorting
    data in the first column and then applying sorting to subsequent columns.

    To disable a specific sorter, \l{Sorter::}{enabled} can be set to \c false.

    The sorter priority can also be overridden by setting the primary sorter
    through the method call \l{SortFilterProxyModel::}{setPrimarySorter()}.
    This would be helpful in the case where the view wants to sort the data of
    any specific column by clicking on the column header such as in
    \l TableView, when there are other sorters also configured for the model.

    The filter \l ValueFilter and \l FunctionFilter can be configured
    in SortFilterProxyModel. Each filter can be set with the
    \l{Filter::}{column} property, similar to the sorter, to filter data in a
    specific column. If no column is specified, then the filter will be applied
    to all the column indexes in the model. To reduce the overhead of unwanted
    checks during filtering, it's recommended to specify the column index.

    To disable a specific filter, \l{Filter::}{enabled} can be set to
    \c false.

    \snippet qml/sortfilterproxymodel/qml-sortfilterproxymodel.qml sfpm-usage

    The AgeFilter in the above code snippet can be declared as follows

    \snippet qml/sortfilterproxymodel/AgeFilter.qml age-filter

    \note This API is considered tech preview and may change or be removed in
    future versions of Qt.
*/
/*!
    \qmlproperty list<Filter> SortFilterProxyModel::filters

    This property holds the list of filters for the \l SortFilterProxyModel.
    If no priority is set, the \l SortFilterProxyModel applies a filter in the
    order as specified in the list.
*/
/*!
    \qmlproperty list<Sorter> SortFilterProxyModel::sorters

    This property holds the list of sorters for the \l SortFilterProxyModel.
    If no priority is set, the \l SortFilterProxyModel applies a sorter in the
    order as specified in the list.
*/

/*!
    \class QQmlSortFilterProxyModel
    \internal
*/
QQmlSortFilterProxyModel::QQmlSortFilterProxyModel(QObject *parent)
    : QAbstractProxyModel (*new QQmlSortFilterProxyModelPrivate, parent)
{
    Q_D(QQmlSortFilterProxyModel);
    d->init();
}

/*!
 *  Clear the filters and sorters and deletes the sort filter proxy model
 */
QQmlSortFilterProxyModel::~QQmlSortFilterProxyModel()
{
    Q_D(QQmlSortFilterProxyModel);
    delete d->m_filters;
    delete d->m_sorters;
}

/*!
 *  Provides the filters configured in the sort filter proxy model
 */
QQmlListProperty<QQmlFilterBase> QQmlSortFilterProxyModel::filters()
{
    Q_D(QQmlSortFilterProxyModel);
    return d->m_filters->filtersListProperty();
}

/*!
 *  Provides the sorters configured in the sort filter proxy model
 */
QQmlListProperty<QQmlSorterBase> QQmlSortFilterProxyModel::sorters()
{
    Q_D(QQmlSortFilterProxyModel);
    return d->m_sorters->sortersListProperty();
}

/*!
    \qmlproperty bool SortFilterProxyModel::dynamicSortFilter

    This property holds whether the proxy model is dynamically sorted
    and filtered whenever the contents of the source model change.

    The default value is \c true.
*/
bool QQmlSortFilterProxyModel::dynamicSortFilter() const
{
    Q_D(const QQmlSortFilterProxyModel);
    return d->m_dynamicSortFilter;
}

void QQmlSortFilterProxyModel::setDynamicSortFilter(const bool enabled)
{
    Q_D(QQmlSortFilterProxyModel);
    if (d->m_dynamicSortFilter == enabled)
        return;
    d->m_dynamicSortFilter = enabled;
    emit dynamicSortFilterChanged();
}

/*!
    \qmlproperty bool SortFilterProxyModel::recursiveFiltering

    This property allows all the configured filters to be applied recursively
    on children. The behavior is similar to that of
    \l {QSortFilterProxyModel::}{recursiveFilteringEnabled} in
    \l QSortFilterProxyModel.

    The default value is \c false.
*/
bool QQmlSortFilterProxyModel::recursiveFiltering() const
{
    Q_D(const QQmlSortFilterProxyModel);
    return d->m_recursiveFiltering;
}

void QQmlSortFilterProxyModel::setRecursiveFiltering(const bool enabled)
{
    Q_D(QQmlSortFilterProxyModel);
    if (d->m_recursiveFiltering == enabled)
        return;
    d->m_recursiveFiltering = enabled;
    emit recursiveFilteringChanged();
}

/*!
    \qmlproperty bool SortFilterProxyModel::autoAcceptChildRows

    This property will not filter out children of accepted rows. The behavior
    is similar to that of \l autoAcceptChildRows in \l QSortFilterProxyModel.

    The default value is \c false.
*/
bool QQmlSortFilterProxyModel::autoAcceptChildRows() const
{
    Q_D(const QQmlSortFilterProxyModel);
    return d->m_autoAcceptChildRows;
}

void QQmlSortFilterProxyModel::setAutoAcceptChildRows(const bool enabled)
{
    Q_D(QQmlSortFilterProxyModel);
    if (d->m_autoAcceptChildRows == enabled)
        return;
    d->m_autoAcceptChildRows = enabled;
    emit autoAcceptChildRowsChanged();
}

/*!
    \qmlproperty var SortFilterProxyModel::model

    This property allows to set source model for the sort filter proxy model.
*/
QVariant QQmlSortFilterProxyModel::model() const
{
    Q_D(const QQmlSortFilterProxyModel);
    return d->m_sourceModel;
}

void QQmlSortFilterProxyModel::setModel(QVariant &model)
{
    Q_D(QQmlSortFilterProxyModel);
    if (d->m_sourceModel == model)
        return;

    auto *itemModel = qobject_cast<QAbstractItemModel *>(qvariant_cast<QObject*>(model));
    if (!itemModel ) {
        qWarning("QQmlSortFilterProxyModel: supports only QAIM for now");
        return;
    }
    d->m_sourceModel = model;
    setSourceModel(itemModel);
    emit modelChanged();
}

/*! internal
 *
 */
void QQmlSortFilterProxyModel::invalidateFilter()
{
    Q_D(QQmlSortFilterProxyModel);
    if (d->m_componentCompleted)
        d->filter_changed();
}

/*!
    \qmlmethod SortFilterProxyModel::invalidate()

    This method invalidates the model by reevaluating the configured filters
    and sorters on the source model data.
 */
void QQmlSortFilterProxyModel::invalidate()
{
    Q_D(QQmlSortFilterProxyModel);
    if (d->m_componentCompleted) {
        d->filter_changed();
        d->sort();
    }
}

/*!
    \qmlmethod SortFilterProxyModel::invalidateSorter()

    This method force the sort filter proxy model to reevaluate the configured
    sorters against the data. It can used in the case where dynamic sorting
    was disabled through property \l dynamicSortFilter
*/
void QQmlSortFilterProxyModel::invalidateSorter()
{
    Q_D(QQmlSortFilterProxyModel);
    if (d->m_componentCompleted)
        d->sort();
}

/*!
    \qmlmethod SortFilterProxyModel::setPrimarySorter(sorter)

    This method allows to set the primary sorter in the sort filter proxy
    model. The primary sorter will be evaluated before all other sorters
    configured as part of \a sorter property. If not configured or passed
    \c null, the sorter with higher priority shall be considered as the
    primary sorter.
*/
void QQmlSortFilterProxyModel::setPrimarySorter(QQmlSorterBase *sorter)
{
    Q_D(QQmlSortFilterProxyModel);
    if (auto *sortCompPriv = static_cast<QQmlSorterCompositorPrivate *>(QQmlSorterCompositorPrivate::get(d->m_sorters))) {
        auto primarySorter = sortCompPriv->primarySorter();
        if (sorter == primarySorter.get())
            return;
        sortCompPriv->setPrimarySorter(sorter);
        emit primarySorterChanged();
        invalidateSorter();
    }
}

/*!
    \internal
 */
void QQmlSortFilterProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    Q_D(QQmlSortFilterProxyModel);

    if (sourceModel == d->model)
        return;

    beginResetModel();

    if (d->model) {
        for (const QMetaObject::Connection &connection : std::as_const(d->m_sourceConnections))
            disconnect(connection);
    }

    // same as in _q_sourceReset()
    d->invalidatePersistentIndexes();
    d->_q_clearMapping();

    QAbstractProxyModel::setSourceModel(sourceModel);

    d->m_sourceConnections = std::array<QMetaObject::Connection, 18>{
        QObjectPrivate::connect(d->model, &QAbstractItemModel::dataChanged, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceDataChanged),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::headerDataChanged, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceHeaderDataChanged),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::rowsAboutToBeInserted, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceRowsAboutToBeInserted),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::rowsInserted, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceRowsInserted),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::columnsAboutToBeInserted, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceColumnsAboutToBeInserted),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::columnsInserted, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceColumnsInserted),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::rowsAboutToBeRemoved, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceRowsAboutToBeRemoved),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::rowsRemoved, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceRowsRemoved),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::columnsAboutToBeRemoved, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceColumnsAboutToBeRemoved),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::columnsRemoved, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceColumnsRemoved),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::rowsAboutToBeMoved, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceRowsAboutToBeMoved),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::rowsMoved, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceRowsMoved),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::columnsAboutToBeMoved, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceColumnsAboutToBeMoved),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::columnsMoved, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceColumnsMoved),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::layoutAboutToBeChanged, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceLayoutAboutToBeChanged),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::layoutChanged, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceLayoutChanged),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::modelAboutToBeReset, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceAboutToBeReset),

        QObjectPrivate::connect(d->model, &QAbstractItemModel::modelReset, d,
                                &QQmlSortFilterProxyModelPrivate::_q_sourceReset)
    };
    endResetModel();

    if (d->m_dynamicSortFilter && d->updatePrimaryColumn())
        d->sort();
}

/*!
    Returns the source model index corresponding to the given \a
    proxyIndex from the sorting filter model.
*/
QModelIndex QQmlSortFilterProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    Q_D(const QQmlSortFilterProxyModel);
    return d->proxy_to_source(proxyIndex);
}

/*!
    Returns the model index in the QQmlSortFilterProxyModel given the \a
    sourceIndex from the source model.
*/
QModelIndex QQmlSortFilterProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    Q_D(const QQmlSortFilterProxyModel);
    return d->source_to_proxy(sourceIndex);
}

/*!
  \reimp
*/
QItemSelection QQmlSortFilterProxyModel::mapSelectionToSource(const QItemSelection &proxySelection) const
{
    return QAbstractProxyModel::mapSelectionToSource(proxySelection);
}

/*!
  \reimp
*/
QItemSelection QQmlSortFilterProxyModel::mapSelectionFromSource(const QItemSelection &sourceSelection) const
{
    return QAbstractProxyModel::mapSelectionFromSource(sourceSelection);
}


/*!
  \reimp
*/
QModelIndex QQmlSortFilterProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const QQmlSortFilterProxyModel);
    if (row < 0 || column < 0)
        return QModelIndex();

    QModelIndex source_parent = mapToSource(parent); // parent is already mapped at this point
    QSortFilterProxyModelHelper::IndexMap::const_iterator it = d->create_mapping(source_parent); // but make sure that the children are mapped
    if (it.value()->source_rows.size() <= row || it.value()->source_columns.size() <= column)
        return QModelIndex();

    return d->createIndex(row, column, it);
}

/*!
  \reimp
*/
QModelIndex QQmlSortFilterProxyModel::parent(const QModelIndex &child) const
{
    Q_D(const QQmlSortFilterProxyModel);
    if (!d->indexValid(child))
        return QModelIndex();
    QSortFilterProxyModelHelper::IndexMap::const_iterator it = d->index_to_iterator(child);
    Q_ASSERT(it != d->source_index_mapping.constEnd());
    QModelIndex source_parent = it.key();
    QModelIndex proxy_parent = mapFromSource(source_parent);
    return proxy_parent;
}

/*!
  \reimp
*/
QModelIndex QQmlSortFilterProxyModel::sibling(int row, int column, const QModelIndex &idx) const
{
    Q_D(const QQmlSortFilterProxyModel);
    if (!d->indexValid(idx))
        return QModelIndex();

    const QSortFilterProxyModelHelper::IndexMap::const_iterator it = d->index_to_iterator(idx);
    if (it.value()->source_rows.size() <= row || it.value()->source_columns.size() <= column)
        return QModelIndex();

    return d->createIndex(row, column, it);
}

/*!
  \reimp
*/
bool QQmlSortFilterProxyModel::hasChildren(const QModelIndex &parent) const
{
    Q_D(const QQmlSortFilterProxyModel);
    QModelIndex source_parent = mapToSource(parent);
    if (parent.isValid() && !source_parent.isValid())
        return false;
    if (!d->model->hasChildren(source_parent))
        return false;

    if (d->model->canFetchMore(source_parent))
        return true; //we assume we might have children that can be fetched

    QSortFilterProxyModelHelper::Mapping *m = d->create_mapping(source_parent).value();
    return m->source_rows.size() != 0 && m->source_columns.size() != 0;
}

int QQmlSortFilterProxyModel::columnCount(const QModelIndex &parent) const
{
    Q_D(const QQmlSortFilterProxyModel);
    if (d->m_sourceModel.isNull() || !d->m_sourceModel.isValid())
        return 0;
    QModelIndex source_parent = mapToSource(parent);
    if (parent.isValid() && !source_parent.isValid())
        return 0;
    QSortFilterProxyModelHelper::IndexMap::const_iterator it = d->create_mapping(source_parent);
    return it.value()->source_columns.size();
}

int QQmlSortFilterProxyModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QQmlSortFilterProxyModel);
    if (d->m_sourceModel.isNull() || !d->m_sourceModel.isValid())
        return 0;
    QModelIndex source_parent = mapToSource(parent);
    if (parent.isValid() && !source_parent.isValid())
        return 0;
    QSortFilterProxyModelHelper::IndexMap::const_iterator it = d->create_mapping(source_parent);
    return it.value()->source_rows.size();
}

QVariant QQmlSortFilterProxyModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QQmlSortFilterProxyModel);
    QModelIndex source_index = mapToSource(index);
    if (index.isValid() && !source_index.isValid())
        return QVariant();
    return d->model->data(source_index, role);
}

bool QQmlSortFilterProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_D(QQmlSortFilterProxyModel);
    QModelIndex source_index = mapToSource(index);
    if (index.isValid() && !source_index.isValid())
        return false;
    return d->model->setData(source_index, value, role);
}

/*!
  \reimp
*/
QVariant QQmlSortFilterProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_D(const QQmlSortFilterProxyModel);
    QSortFilterProxyModelHelper::IndexMap::const_iterator it = d->create_mapping(QModelIndex());
    if (it.value()->source_rows.size() * it.value()->source_columns.size() > 0)
        return QAbstractProxyModel::headerData(section, orientation, role);
    int source_section;
    if (orientation == Qt::Vertical) {
        if (section < 0 || section >= it.value()->source_rows.size())
            return QVariant();
        source_section = it.value()->source_rows.at(section);
    } else {
        if (section < 0 || section >= it.value()->source_columns.size())
            return QVariant();
        source_section = it.value()->source_columns.at(section);
    }
    return d->model->headerData(source_section, orientation, role);
}

/*!
  \reimp
*/
bool QQmlSortFilterProxyModel::setHeaderData(int section, Qt::Orientation orientation,
                                              const QVariant &value, int role)
{
    Q_D(QQmlSortFilterProxyModel);
    QSortFilterProxyModelHelper::IndexMap::const_iterator it = d->create_mapping(QModelIndex());
    if (it.value()->source_rows.size() * it.value()->source_columns.size() > 0)
        return QAbstractProxyModel::setHeaderData(section, orientation, value, role);
    int source_section;
    if (orientation == Qt::Vertical) {
        if (section < 0 || section >= it.value()->source_rows.size())
            return false;
        source_section = it.value()->source_rows.at(section);
    } else {
        if (section < 0 || section >= it.value()->source_columns.size())
            return false;
        source_section = it.value()->source_columns.at(section);
    }
    return d->model->setHeaderData(source_section, orientation, value, role);
}

/*!
  \reimp
*/
bool QQmlSortFilterProxyModel::insertRows(int row, int count, const QModelIndex &parent)
{
    Q_D(QQmlSortFilterProxyModel);
    if (row < 0 || count <= 0)
        return false;
    QModelIndex source_parent = mapToSource(parent);
    if (parent.isValid() && !source_parent.isValid())
        return false;
    QSortFilterProxyModelHelper::Mapping *m = d->create_mapping(source_parent).value();
    if (row > m->source_rows.size())
        return false;
    int source_row = (row >= m->source_rows.size()
                              ? m->proxy_rows.size()
                              : m->source_rows.at(row));
    return d->model->insertRows(source_row, count, source_parent);
}

/*!
  \reimp
*/
bool QQmlSortFilterProxyModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QQmlSortFilterProxyModel);
    if (column < 0|| count <= 0)
        return false;
    QModelIndex source_parent = mapToSource(parent);
    if (parent.isValid() && !source_parent.isValid())
        return false;
    QSortFilterProxyModelHelper::Mapping *m = d->create_mapping(source_parent).value();
    if (column > m->source_columns.size())
        return false;
    int source_column = (column >= m->source_columns.size()
                                 ? m->proxy_columns.size()
                                 : m->source_columns.at(column));
    return d->model->insertColumns(source_column, count, source_parent);
}

/*!
  \reimp
*/
bool QQmlSortFilterProxyModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_D(QQmlSortFilterProxyModel);
    if (row < 0 || count <= 0)
        return false;
    QModelIndex source_parent = mapToSource(parent);
    if (parent.isValid() && !source_parent.isValid())
        return false;
    QSortFilterProxyModelHelper::Mapping *m = d->create_mapping(source_parent).value();
    if (row + count > m->source_rows.size())
        return false;
    if ((count == 1)
        || ((d->m_primarySortColumn < 0) && (m->proxy_rows.size() == m->source_rows.size()))) {
        int source_row = m->source_rows.at(row);
        return d->model->removeRows(source_row, count, source_parent);
    }
    // remove corresponding source intervals
    // ### if this proves to be slow, we can switch to single-row removal
    QList<int> rows;
    rows.reserve(count);
    for (int i = row; i < row + count; ++i)
        rows.append(m->source_rows.at(i));
    std::sort(rows.begin(), rows.end());

    int pos = rows.size() - 1;
    bool ok = true;
    while (pos >= 0) {
        const int source_end = rows.at(pos--);
        int source_start = source_end;
        while ((pos >= 0) && (rows.at(pos) == (source_start - 1))) {
            --source_start;
            --pos;
        }
        ok = ok && d->model->removeRows(source_start, source_end - source_start + 1,
                                        source_parent);
    }
    return ok;
}

/*!
  \reimp
*/
bool QQmlSortFilterProxyModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    Q_D(QQmlSortFilterProxyModel);
    if (column < 0 || count <= 0)
        return false;
    QModelIndex source_parent = mapToSource(parent);
    if (parent.isValid() && !source_parent.isValid())
        return false;
    QSortFilterProxyModelHelper::Mapping *m = d->create_mapping(source_parent).value();
    if (column + count > m->source_columns.size())
        return false;
    if ((count == 1) || (m->proxy_columns.size() == m->source_columns.size())) {
        int source_column = m->source_columns.at(column);
        return d->model->removeColumns(source_column, count, source_parent);
    }
    // remove corresponding source intervals
    QList<int> columns;
    columns.reserve(count);
    for (int i = column; i < column + count; ++i)
        columns.append(m->source_columns.at(i));

    int pos = columns.size() - 1;
    bool ok = true;
    while (pos >= 0) {
        const int source_end = columns.at(pos--);
        int source_start = source_end;
        while ((pos >= 0) && (columns.at(pos) == (source_start - 1))) {
            --source_start;
            --pos;
        }
        ok = ok && d->model->removeColumns(source_start, source_end - source_start + 1,
                                           source_parent);
    }
    return ok;
}

/*!
    \internal
 */
QHash<int, QByteArray> QQmlSortFilterProxyModel::roleNames() const
{
    if (const auto srcModel = sourceModel())
        return srcModel->roleNames();
    return {};
}

/*!
    \internal
 */
QVariant QQmlSortFilterProxyModel::sourceData(const QModelIndex &sourceIndex, int role) const
{
    return sourceModel()->data(sourceIndex, role);
}

/*!
    \internal
 */
bool QQmlSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex& sourceParent) const
{
    Q_D(const QQmlSortFilterProxyModel);
    if (!d->m_componentCompleted)
        return true;
    return d->m_filters->filterAcceptsRowInternal(row, sourceParent, this);
}

/*!
    \internal
 */
bool QQmlSortFilterProxyModel::filterAcceptsColumn(int column, const QModelIndex &sourceParent) const
{
    Q_D(const QQmlSortFilterProxyModel);
    if (!d->m_componentCompleted)
        return true;
    return d->m_filters->filterAcceptsColumnInternal(column, sourceParent, this);
}

/*!
    \internal
 */
bool QQmlSortFilterProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
    Q_D(const QQmlSortFilterProxyModel);
    if (d->m_sorters)
        return d->m_sorters->lessThan(sourceLeft, sourceRight, this);
    return false;
}

/*!
    \internal
 */
void QQmlSortFilterProxyModel::componentComplete()
{
    // We need to explicitly call sort here. This is because, as of now the
    // actual implementation for sorting comes from QSortFilterProxyModelBase
    // and it trigger the sorting operation with the corresponding column set
    // and it happens only after parsing the configured sorters.
    Q_D(QQmlSortFilterProxyModel);
    d->m_componentCompleted = true;
    d->filter_changed();
    if (d->m_dynamicSortFilter)
        d->sort();
}

/*!
    \internal
 */
void QQmlSortFilterProxyModel::sort(int column, Qt::SortOrder order)
{
    Q_D(QQmlSortFilterProxyModel);
    if (!d->m_componentCompleted ||
        (d->m_dynamicSortFilter && d->m_proxySortColumn == column && d->m_sortOrder == order))
        return;
    d->m_sortOrder = order;
    d->m_proxySortColumn = column;
    d->updatePrimaryColumn();
    d->sort();
}

/*!
    \internal
 */
int QQmlSortFilterProxyModel::itemRoleForName(const QString& roleName) const
{
    QHash<int, QByteArray> itemRoleNames = roleNames();
    return !itemRoleNames.isEmpty() ? itemRoleNames.key(roleName.toUtf8(), -1) : -1;
}

/*!
    \internal
 */
void QQmlSortFilterProxyModel::setPrimarySortColumn(const int column)
{
    Q_D(QQmlSortFilterProxyModel);
    d->m_proxySortColumn = column;
    d->updatePrimaryColumn();
}

/*!
    \internal
 */
void QQmlSortFilterProxyModel::setPrimarySortOrder(const Qt::SortOrder order)
{
    Q_D(QQmlSortFilterProxyModel);
    d->m_sortOrder = order;
}

/*!
    \internal
 */
void QQmlSortFilterProxyModelPrivate::init()
{
    Q_Q(QQmlSortFilterProxyModel);
    m_filters = new QQmlFilterCompositor(q);
    QObject::connect(q, &QQmlSortFilterProxyModel::filtersChanged,
                     q, &QQmlSortFilterProxyModel::invalidateFilter);
    m_sorters = new QQmlSorterCompositor(q);
    QObject::connect(q, &QQmlSortFilterProxyModel::sortersChanged,
                     q, &QQmlSortFilterProxyModel::invalidateSorter);
}

void QQmlSortFilterProxyModelPrivate::_q_clearMapping()
{
    // store the persistent indexes
    QModelIndexPairList source_indexes = store_persistent_indexes();

    clearSourceIndexMapping();
    if (m_dynamicSortFilter)
        m_primarySortColumn = findPrimarySortColumn();

    // update the persistent indexes
    update_persistent_indexes(source_indexes);
}

bool QQmlSortFilterProxyModelPrivate::recursiveParentAcceptsRow(const QModelIndex &source_parent) const
{
    Q_Q(const QQmlSortFilterProxyModel);
    if (source_parent.isValid()) {
        const QModelIndex index = source_parent.parent();
        if (q->filterAcceptsRow(source_parent.row(), index))
            return true;
        return recursiveParentAcceptsRow(index);
    }
    return false;
}

bool QQmlSortFilterProxyModelPrivate::recursiveChildAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_Q(const QQmlSortFilterProxyModel);
    const QModelIndex index = model->index(source_row, 0, source_parent);
    const int count = model->rowCount(index);
    for (int i = 0; i < count; ++i) {
        if (q->filterAcceptsRow(i, index))
            return true;
        if (recursiveChildAcceptsRow(i, index))
            return true;
    }
    return false;
}

/*!
    \internal

    Update the primary sort column according to the m_proxySortColumn
    return true if the column was changed
*/
bool QQmlSortFilterProxyModelPrivate::updatePrimaryColumn()
{
    int old_primayColumn = m_primarySortColumn;

    if (m_proxySortColumn == -1) {
        m_primarySortColumn = -1;
    } else {
        // We cannot use index mapping here because in case of a still-empty
        // proxy model there's no valid proxy index we could map to source.
        // So always use the root mapping directly instead.
        QSortFilterProxyModelHelper::Mapping *m = create_mapping(QModelIndex()).value();
        if (m_proxySortColumn < m->source_columns.size())
            m_primarySortColumn = m->source_columns.at(m_proxySortColumn);
        else
            m_primarySortColumn = -1;
    }

    return old_primayColumn != m_primarySortColumn;
}

/*!
    \internal

    Find the primary sort column without creating a full mapping and
    without updating anything.
*/
int QQmlSortFilterProxyModelPrivate::findPrimarySortColumn() const
{
    if (m_proxySortColumn == -1)
        return -1;

    const QModelIndex rootIndex;
    const int source_cols = model->columnCount();
    int accepted_columns = -1;

    Q_Q(const QQmlSortFilterProxyModel);
    for (int i = 0; i < source_cols; ++i) {
        if (q->filterAcceptsColumn(i, rootIndex)) {
            if (++accepted_columns == m_proxySortColumn)
                return i;
        }
    }

    return -1;
}

/*!
    \internal

    Sorts the given \a source_rows according to current sort column and order.
*/
void QQmlSortFilterProxyModelPrivate::sort_source_rows(
        QList<int> &source_rows, const QModelIndex &source_parent) const
{
    if (m_primarySortColumn >= 0) {
        if (m_sortOrder == Qt::AscendingOrder) {
            QSortFilterProxyModelLessThan lt(m_primarySortColumn, source_parent, model, this);
            std::stable_sort(source_rows.begin(), source_rows.end(), lt);
        } else {
            QSortFilterProxyModelGreaterThan gt(m_primarySortColumn, source_parent, model, this);
            std::stable_sort(source_rows.begin(), source_rows.end(), gt);
        }
    } else if (m_sortOrder == Qt::AscendingOrder) {
        std::stable_sort(source_rows.begin(), source_rows.end(), std::less{});
    } else {
        std::stable_sort(source_rows.begin(), source_rows.end(), std::greater{});
    }
}

/*!
    \internal

    Given proxy-to-source mapping \a proxy_to_source and a set of
    unmapped source items \a source_items, determines the proxy item
    intervals at which the subsets of source items should be inserted
    (but does not actually add them to the mapping).

    The result is a vector of pairs, each pair representing a tuple (start,
    items), where items is a vector containing the (sorted) source items that
    should be inserted at that proxy model location.
*/
QList<std::pair<int, QList<int>>> QQmlSortFilterProxyModelPrivate::proxy_intervals_for_source_items_to_add(
        const QList<int> &proxy_to_source, const QList<int> &source_items,
        const QModelIndex &source_parent, QSortFilterProxyModelHelper::Direction direction) const
{
    Q_Q(const QQmlSortFilterProxyModel);
    QList<std::pair<int, QList<int>>> proxy_intervals;
    if (source_items.isEmpty())
        return proxy_intervals;

    int proxy_low = 0;
    int proxy_item = 0;
    int source_items_index = 0;
    bool compare = (direction == QSortFilterProxyModelHelper::Direction::Rows && m_primarySortColumn >= 0 && m_dynamicSortFilter);
    while (source_items_index < source_items.size()) {
        QList<int> source_items_in_interval;
        int first_new_source_item = source_items.at(source_items_index);
        source_items_in_interval.append(first_new_source_item);
        ++source_items_index;

        // Find proxy item at which insertion should be started
        int proxy_high = proxy_to_source.size() - 1;
        QModelIndex i1 = compare ? model->index(first_new_source_item, m_primarySortColumn, source_parent) : QModelIndex();
        while (proxy_low <= proxy_high) {
            proxy_item = (proxy_low + proxy_high) / 2;
            if (compare) {
                QModelIndex i2 = model->index(proxy_to_source.at(proxy_item), m_primarySortColumn, source_parent);
                if ((m_sortOrder == Qt::AscendingOrder) ? q->lessThan(i1, i2) : q->lessThan(i2, i1))
                    proxy_high = proxy_item - 1;
                else
                    proxy_low = proxy_item + 1;
            } else {
                if (first_new_source_item < proxy_to_source.at(proxy_item))
                    proxy_high = proxy_item - 1;
                else
                    proxy_low = proxy_item + 1;
            }
        }
        proxy_item = proxy_low;

        // Find the sequence of new source items that should be inserted here
        if (proxy_item >= proxy_to_source.size()) {
            for ( ; source_items_index < source_items.size(); ++source_items_index)
                source_items_in_interval.append(source_items.at(source_items_index));
        } else {
            i1 = compare ? model->index(proxy_to_source.at(proxy_item), m_primarySortColumn, source_parent) : QModelIndex();
            for ( ; source_items_index < source_items.size(); ++source_items_index) {
                int new_source_item = source_items.at(source_items_index);
                if (compare) {
                    QModelIndex i2 = model->index(new_source_item, m_primarySortColumn, source_parent);
                    if ((m_sortOrder == Qt::AscendingOrder) ? q->lessThan(i1, i2) : q->lessThan(i2, i1))
                        break;
                } else {
                    if (proxy_to_source.at(proxy_item) < new_source_item)
                        break;
                }
                source_items_in_interval.append(new_source_item);
            }
        }
        // Add interval to result
        proxy_intervals.emplace_back(proxy_item, std::move(source_items_in_interval));
    }
    return proxy_intervals;
}

bool QQmlSortFilterProxyModelPrivate::needsReorder(const QList<int> &source_rows, const QModelIndex &source_parent) const
{
    Q_Q(const QQmlSortFilterProxyModel);
    Q_ASSERT(m_primarySortColumn != -1);
    const int proxyRowCount = q->rowCount(source_to_proxy(source_parent));
    // If any modified proxy row no longer passes lessThan(previous, current)
    // or lessThan(current, next) then we need to reorder.
    return std::any_of(source_rows.begin(), source_rows.end(),
                       [this, q, proxyRowCount, source_parent](int sourceRow) -> bool {
                           const QModelIndex sourceIndex = model->index(sourceRow, m_primarySortColumn, source_parent);
                           const QModelIndex proxyIndex = source_to_proxy(sourceIndex);
                           Q_ASSERT(proxyIndex.isValid()); // caller ensured source_rows were not filtered out
                           if (proxyIndex.row() > 0) {
                               const QModelIndex prevProxyIndex = q->sibling(proxyIndex.row() - 1, m_proxySortColumn, proxyIndex);
                               const QModelIndex prevSourceIndex = proxy_to_source(prevProxyIndex);
                               if (m_sortOrder == Qt::AscendingOrder ? q->lessThan(sourceIndex, prevSourceIndex) : q->lessThan(prevSourceIndex, sourceIndex))
                                   return true;
                           }
                           if (proxyIndex.row() < proxyRowCount - 1) {
                               const QModelIndex nextProxyIndex = q->sibling(proxyIndex.row() + 1, m_proxySortColumn, proxyIndex);
                               const QModelIndex nextSourceIndex = proxy_to_source(nextProxyIndex);
                               if (m_sortOrder == Qt::AscendingOrder ? q->lessThan(nextSourceIndex, sourceIndex) : q->lessThan(sourceIndex, nextSourceIndex))
                                   return true;
                           }
                           return false;
                       });
}

void QQmlSortFilterProxyModelPrivate::_q_sourceDataChanged(const QModelIndex &source_top_left,
                                                            const QModelIndex &source_bottom_right,
                                                            const QList<int> &roles)
{
    Q_Q(QQmlSortFilterProxyModel);
    if (!source_top_left.isValid() || !source_bottom_right.isValid())
        return;

    std::vector<QSortFilterProxyModelDataChanged> data_changed_list;
    data_changed_list.emplace_back(source_top_left, source_bottom_right);

    // Do check parents if the filter role have changed and we are recursive
    if (containRoleForRecursiveFilter(roles)) {
        QModelIndex source_parent = source_top_left.parent();
        while (source_parent.isValid()) {
            data_changed_list.emplace_back(source_parent, source_parent);
            source_parent = source_parent.parent();
        }
    }

    for (const QSortFilterProxyModelDataChanged &data_changed : data_changed_list) {
        const QModelIndex &source_top_left = data_changed.topLeft;
        const QModelIndex &source_bottom_right = data_changed.bottomRight;
        const QModelIndex source_parent = source_top_left.parent();

        bool change_in_unmapped_parent = false;
        QSortFilterProxyModelHelper::IndexMap::const_iterator it = source_index_mapping.constFind(source_parent);
        if (it == source_index_mapping.constEnd()) {
            // We don't have mapping for this index, so we cannot know how
            // things changed (in case the change affects filtering) in order
            // to forward the change correctly.
            // But we can at least forward the signal "as is", if the row isn't
            // filtered out, this is better than nothing.
            it = create_mapping_recursive(source_parent);
            if (it == source_index_mapping.constEnd())
                continue;
            change_in_unmapped_parent = true;
        }

        QSortFilterProxyModelHelper::Mapping *m = it.value();

        // Figure out how the source changes affect us
        QList<int> source_rows_remove;
        QList<int> source_rows_insert;
        QList<int> source_rows_change;
        QList<int> source_rows_resort;
        int end = qMin(source_bottom_right.row(), m->proxy_rows.size() - 1);
        for (int source_row = source_top_left.row(); source_row <= end; ++source_row) {
            if (m_dynamicSortFilter && !change_in_unmapped_parent) {
                if (m->proxy_rows.at(source_row) != -1) {
                    if (!filterAcceptsRowInternal(source_row, source_parent)) {
                        // This source row no longer satisfies the filter, so
                        // it must be removed
                        source_rows_remove.append(source_row);
                    } else if (m_primarySortColumn >= source_top_left.column() && m_primarySortColumn <= source_bottom_right.column()) {
                        // This source row has changed in a way that may affect
                        // sorted order
                        source_rows_resort.append(source_row);
                    } else {
                        // This row has simply changed, without affecting
                        // filtering nor sorting
                        source_rows_change.append(source_row);
                    }
                } else {
                    if (!m_itemsBeingRemoved.contains(source_parent, source_row) && filterAcceptsRowInternal(source_row, source_parent)) {
                        // This source row now satisfies the filter, so it must
                        // be added
                        source_rows_insert.append(source_row);
                    }
                }
            } else {
                if (m->proxy_rows.at(source_row) != -1)
                    source_rows_change.append(source_row);
            }
        }

        if (!source_rows_remove.isEmpty()) {
            remove_source_items(m->proxy_rows, m->source_rows,
                                source_rows_remove, source_parent, QSortFilterProxyModelHelper::Direction::Rows);
            QSet<int> source_rows_remove_set = qListToSet(source_rows_remove);
            QList<QModelIndex>::iterator childIt = m->mapped_children.end();
            while (childIt != m->mapped_children.begin()) {
                --childIt;
                const QModelIndex source_child_index = *childIt;
                if (source_rows_remove_set.contains(source_child_index.row())) {
                    childIt = m->mapped_children.erase(childIt);
                    remove_from_mapping(source_child_index);
                }
            }
        }

        if (!source_rows_resort.isEmpty()) {
            if (needsReorder(source_rows_resort, source_parent)) {
                // Re-sort the rows of this level
                QList<QPersistentModelIndex> parents;
                parents << q->mapFromSource(source_parent);
                emit q->layoutAboutToBeChanged(parents, QAbstractItemModel::VerticalSortHint);
                QModelIndexPairList source_indexes = store_persistent_indexes();
                remove_source_items(m->proxy_rows, m->source_rows, source_rows_resort,
                                    source_parent, QSortFilterProxyModelHelper::Direction::Rows, false);
                sort_source_rows(source_rows_resort, source_parent);
                insert_source_items(m->proxy_rows, m->source_rows, source_rows_resort,
                                    source_parent, QSortFilterProxyModelHelper::Direction::Rows, false);
                update_persistent_indexes(source_indexes);
                emit q->layoutChanged(parents, QAbstractItemModel::VerticalSortHint);
            }
            // Make sure we also emit dataChanged for the rows
            source_rows_change += source_rows_resort;
        }

        if (!source_rows_change.isEmpty()) {
            // Find the proxy row range
            int proxy_start_row;
            int proxy_end_row;
            proxy_item_range(m->proxy_rows, source_rows_change,
                             proxy_start_row, proxy_end_row);
            // ### Find the proxy column range also
            if (proxy_end_row >= 0) {
                // the row was accepted, but some columns might still be
                // filtered out
                int source_left_column = source_top_left.column();
                while (source_left_column < source_bottom_right.column()
                       && m->proxy_columns.at(source_left_column) == -1)
                    ++source_left_column;
                if (m->proxy_columns.at(source_left_column) != -1) {
                    const QModelIndex proxy_top_left = createIndex(
                            proxy_start_row, m->proxy_columns.at(source_left_column), it);
                    int source_right_column = source_bottom_right.column();
                    while (source_right_column > source_top_left.column()
                           && m->proxy_columns.at(source_right_column) == -1)
                        --source_right_column;
                    if (m->proxy_columns.at(source_right_column) != -1) {
                        const QModelIndex proxy_bottom_right = createIndex(
                                proxy_end_row, m->proxy_columns.at(source_right_column), it);
                        emit q->dataChanged(proxy_top_left, proxy_bottom_right, roles);
                    }
                }
            }
        }

        if (!source_rows_insert.isEmpty()) {
            sort_source_rows(source_rows_insert, source_parent);
            insert_source_items(m->proxy_rows, m->source_rows,
                                source_rows_insert, source_parent, QSortFilterProxyModelHelper::Direction::Rows);
        }
    }
}

void QQmlSortFilterProxyModelPrivate::_q_sourceHeaderDataChanged(Qt::Orientation orientation,
                                                                  int start, int end)
{
    Q_ASSERT(start <= end);

    Q_Q(QQmlSortFilterProxyModel);
    QSortFilterProxyModelHelper::Mapping *m = create_mapping(QModelIndex()).value();

    const QList<int> &source_to_proxy = (orientation == Qt::Vertical) ? m->proxy_rows : m->proxy_columns;

    QList<int> proxy_positions;
    proxy_positions.reserve(end - start + 1);
    {
        Q_ASSERT(source_to_proxy.size() > end);
        QList<int>::const_iterator it = source_to_proxy.constBegin() + start;
        const QList<int>::const_iterator endIt = source_to_proxy.constBegin() + end + 1;
        for ( ; it != endIt; ++it) {
            if (*it != -1)
                proxy_positions.push_back(*it);
        }
    }

    std::sort(proxy_positions.begin(), proxy_positions.end());

    int last_index = 0;
    const int numItems = proxy_positions.size();
    while (last_index < numItems) {
        const int proxyStart = proxy_positions.at(last_index);
        int proxyEnd = proxyStart;
        ++last_index;
        for (int i = last_index; i < numItems; ++i) {
            if (proxy_positions.at(i) == proxyEnd + 1) {
                ++last_index;
                ++proxyEnd;
            } else {
                break;
            }
        }
        emit q->headerDataChanged(orientation, proxyStart, proxyEnd);
    }
}

void QQmlSortFilterProxyModelPrivate::_q_sourceAboutToBeReset()
{
    Q_Q(QQmlSortFilterProxyModel);
    q->beginResetModel();
}

void QQmlSortFilterProxyModelPrivate::_q_sourceReset()
{
    Q_Q(QQmlSortFilterProxyModel);
    invalidatePersistentIndexes();
    _q_clearMapping();
    // All internal structures are deleted in clear()
    q->endResetModel();
    if (updatePrimaryColumn() && m_dynamicSortFilter)
        sort();
}

void QQmlSortFilterProxyModelPrivate::_q_sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_Q(QQmlSortFilterProxyModel);
    Q_UNUSED(hint); // We can't forward Hint because we might filter additional rows or columns
    m_savedPersistentIndexes.clear();

    m_savedLayoutChangeParents.clear();
    for (const QPersistentModelIndex &parent : sourceParents) {
        if (!parent.isValid()) {
            m_savedLayoutChangeParents << QPersistentModelIndex();
            continue;
        }
        const QModelIndex mappedParent = q->mapFromSource(parent);
        // Might be filtered out.
        if (mappedParent.isValid())
            m_savedLayoutChangeParents << mappedParent;
    }

    // All parents filtered out.
    if (!sourceParents.isEmpty() && m_savedLayoutChangeParents.isEmpty())
        return;

    emit q->layoutAboutToBeChanged(m_savedLayoutChangeParents);
    if (persistent.indexes.isEmpty())
        return;

    m_savedPersistentIndexes = store_persistent_indexes();
}

void QQmlSortFilterProxyModelPrivate::_q_sourceLayoutChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_Q(QQmlSortFilterProxyModel);
    Q_UNUSED(hint); // We can't forward Hint because we might filter additional rows or columns

    if (!sourceParents.isEmpty() && m_savedLayoutChangeParents.isEmpty())
        return;

    // Optimize: We only actually have to clear the mapping related to the
    // contents of sourceParents, not everything.
    clearSourceIndexMapping();

    update_persistent_indexes(m_savedPersistentIndexes);
    m_savedPersistentIndexes.clear();

    if (m_dynamicSortFilter)
        m_primarySortColumn = findPrimarySortColumn();

    emit q->layoutChanged(m_savedLayoutChangeParents);
    m_savedLayoutChangeParents.clear();
}

void QQmlSortFilterProxyModelPrivate::_q_sourceRowsAboutToBeInserted(
        const QModelIndex &source_parent, int start, int end)
{
    Q_UNUSED(start);
    Q_UNUSED(end);

    const bool toplevel = !source_parent.isValid();
    const bool recursive_accepted = m_recursiveFiltering && !toplevel && filterAcceptsRowInternal(source_parent.row(), source_parent.parent());
    // Force the creation of a mapping now, even if it's empty.
    // We need it because the proxy can be accessed at the moment it emits
    // rowsAboutToBeInserted in insert_source_items
    if (!m_recursiveFiltering || toplevel || recursive_accepted) {
        if (can_create_mapping(source_parent))
            create_mapping(source_parent);
        if (m_recursiveFiltering)
            m_completeInsert = true;
    } else {
        // The row could have been rejected or the parent might be not yet
        // known... let's try to discover it
        QModelIndex top_source_parent = source_parent;
        QModelIndex parent = source_parent.parent();
        QModelIndex grandParent = parent.parent();

        while (parent.isValid() && !filterAcceptsRowInternal(parent.row(), grandParent)) {
            top_source_parent = parent;
            parent = grandParent;
            grandParent = parent.parent();
        }

        m_lastTopSource = top_source_parent;
    }
}

void QQmlSortFilterProxyModelPrivate::_q_sourceRowsInserted(
        const QModelIndex &source_parent, int start, int end)
{
    if (!m_recursiveFiltering || m_completeInsert) {
        if (m_recursiveFiltering)
            m_completeInsert = false;
        source_items_inserted(source_parent, start, end, QSortFilterProxyModelHelper::Direction::Rows);
        if (updatePrimaryColumn() && m_dynamicSortFilter) //previous call to updatePrimaryColumn may fail if the model has no column.
            sort(); // now it should succeed so we need to make sure to sort again
        return;
    }
    if (m_recursiveFiltering) {
        bool accept = false;
        for (int row = start; row <= end; ++row) {
            if (filterAcceptsRowInternal(row, source_parent)) {
                accept = true;
                break;
            }
        }
        if (!accept) // the new rows have no descendants that match the filter, filter them out.
            return;
        // m_lastTopSource should now become visible
        _q_sourceDataChanged(m_lastTopSource, m_lastTopSource, QList<int>());
    }
}

void QQmlSortFilterProxyModelPrivate::_q_sourceRowsAboutToBeRemoved(
        const QModelIndex &source_parent, int start, int end)
{
    m_itemsBeingRemoved = QRowsRemoval(source_parent, start, end);
    source_items_about_to_be_removed(source_parent, start, end,
                                     QSortFilterProxyModelHelper::Direction::Rows);
}

void QQmlSortFilterProxyModelPrivate::_q_sourceRowsRemoved(
        const QModelIndex &source_parent, int start, int end)
{
    m_itemsBeingRemoved = QRowsRemoval();
    source_items_removed(source_parent, start, end, QSortFilterProxyModelHelper::Direction::Rows);

    if (m_recursiveFiltering) {
        // Find out if removing this visible row means that some ascendant
        // row can now be hidden.
        // We go up until we find a row that should still be visible
        // and then make QSFPM re-evaluate the last one we saw before that,
        // to hide it.
        QModelIndex to_hide;
        QModelIndex source_ascendant = source_parent;

        while (source_ascendant.isValid()) {
            if (filterAcceptsRowInternal(source_ascendant.row(), source_ascendant.parent()))
                break;

            to_hide = source_ascendant;
            source_ascendant = source_ascendant.parent();
        }

        if (to_hide.isValid())
            _q_sourceDataChanged(to_hide, to_hide, QList<int>());
    }
}

void QQmlSortFilterProxyModelPrivate::_q_sourceRowsAboutToBeMoved(
        const QModelIndex &sourceParent, int /* sourceStart */, int /* sourceEnd */, const QModelIndex &destParent, int /* dest */)
{
    // Because rows which are contiguous in the source model might not be
    // contiguous in the proxy due to sorting, the best thing we can do here is
    // be specific about what parents are having their children changed.
    // Optimize: Emit move signals if the proxy is not sorted. Will need to
    // account for rows being filtered out though.
    QList<QPersistentModelIndex> parents;
    parents << sourceParent;
    if (sourceParent != destParent)
        parents << destParent;
    _q_sourceLayoutAboutToBeChanged(parents, QAbstractItemModel::NoLayoutChangeHint);
}

void QQmlSortFilterProxyModelPrivate::_q_sourceRowsMoved(
        const QModelIndex &sourceParent, int /* sourceStart */, int /* sourceEnd */, const QModelIndex &destParent, int /* dest */)
{
    QList<QPersistentModelIndex> parents;
    parents << sourceParent;
    if (sourceParent != destParent)
        parents << destParent;
    _q_sourceLayoutChanged(parents, QAbstractItemModel::NoLayoutChangeHint);
}

void QQmlSortFilterProxyModelPrivate::_q_sourceColumnsAboutToBeInserted(
        const QModelIndex &source_parent, int start, int end)
{
    Q_UNUSED(start);
    Q_UNUSED(end);
    // Force the creation of a mapping now, even if it's empty.
    // We need it because the proxy can be accessed at the moment it emits
    // columnsAboutToBeInserted in insert_source_items
    if (can_create_mapping(source_parent))
        create_mapping(source_parent);
}

void QQmlSortFilterProxyModelPrivate::_q_sourceColumnsInserted(
        const QModelIndex &source_parent, int start, int end)
{
    Q_Q(const QQmlSortFilterProxyModel);
    source_items_inserted(source_parent, start, end, QSortFilterProxyModelHelper::Direction::Columns);

    if (source_parent.isValid())
        return; //we sort according to the root column only
    if (m_primarySortColumn == -1) {
        //we update the primayColumn depending on the m_proxySortColumn
        if (updatePrimaryColumn() && m_dynamicSortFilter)
            sort();
    } else {
        if (start <= m_primarySortColumn)
            m_primarySortColumn += end - start + 1;

        m_proxySortColumn = q->mapFromSource(model->index(0,m_primarySortColumn, source_parent)).column();
    }
}

void QQmlSortFilterProxyModelPrivate::_q_sourceColumnsAboutToBeRemoved(
        const QModelIndex &source_parent, int start, int end)
{
    source_items_about_to_be_removed(source_parent, start, end,
                                     QSortFilterProxyModelHelper::Direction::Columns);
}

void QQmlSortFilterProxyModelPrivate::_q_sourceColumnsRemoved(
        const QModelIndex &source_parent, int start, int end)
{
    Q_Q(const QQmlSortFilterProxyModel);
    source_items_removed(source_parent, start, end, QSortFilterProxyModelHelper::Direction::Columns);

    if (source_parent.isValid())
        return; //we sort according to the root column only
    if (start <= m_primarySortColumn) {
        if (end < m_primarySortColumn)
            m_primarySortColumn -= end - start + 1;
        else
            m_primarySortColumn = -1;
    }

    if (m_primarySortColumn >= 0)
        m_proxySortColumn = q->mapFromSource(model->index(0,m_primarySortColumn, source_parent)).column();
    else
        m_proxySortColumn = -1;
}

void QQmlSortFilterProxyModelPrivate::_q_sourceColumnsAboutToBeMoved(
        const QModelIndex &sourceParent, int /* sourceStart */, int /* sourceEnd */, const QModelIndex &destParent, int /* dest */)
{
    QList<QPersistentModelIndex> parents;
    parents << sourceParent;
    if (sourceParent != destParent)
        parents << destParent;
    _q_sourceLayoutAboutToBeChanged(parents, QAbstractItemModel::NoLayoutChangeHint);
}

void QQmlSortFilterProxyModelPrivate::_q_sourceColumnsMoved(
        const QModelIndex &sourceParent, int /* sourceStart */, int /* sourceEnd */, const QModelIndex &destParent, int /* dest */)
{
    QList<QPersistentModelIndex> parents;
    parents << sourceParent;
    if (sourceParent != destParent)
        parents << destParent;
    _q_sourceLayoutChanged(parents, QAbstractItemModel::NoLayoutChangeHint);
}

/*!
    \internal
 */
bool QQmlSortFilterProxyModelPrivate::containRoleForRecursiveFilter(const QList<int> &roles) const
{
    if (!model || roles.isEmpty())
        return false;
    // Get role names for the provided roles (roles arg.)
    const QHash<int, QByteArray> &roleNames = model->roleNames();
    QList<QString> filterRoles;
    std::for_each(roles.constBegin(), roles.constEnd(), [&roleNames, &filterRoles](const int role){
        if (roleNames.contains(role))
            filterRoles.append(QString::fromUtf8(roleNames[role]));
    });

    // Check if the configured role filter matches with the provided roles
    bool filterRoleConfigured = false;
    if (!filterRoles.isEmpty() && (m_filters && m_filters->filters().isEmpty())) {
        const QList<QQmlFilterBase *> proxyFilters = m_filters->filters();
        filterRoleConfigured = std::any_of(proxyFilters.constBegin(), proxyFilters.constEnd(), [&filterRoles](QQmlFilterBase* filterComp) {
            if (const auto *roleFilter = qobject_cast<QQmlRoleFilter *>(filterComp))
                return filterRoles.contains(roleFilter->roleName());
            return true;
        });
    }
    return m_recursiveFiltering && filterRoleConfigured;
}

/*!
    \internal
 */
bool QQmlSortFilterProxyModelPrivate::filterAcceptsRowInternal(int row, const QModelIndex &sourceIndex) const
{
    Q_Q(const QQmlSortFilterProxyModel);
    const bool retVal = (q->filterAcceptsRow(row, sourceIndex) ||
                         (m_autoAcceptChildRows && recursiveParentAcceptsRow(sourceIndex)) ||
                         (m_recursiveFiltering && recursiveChildAcceptsRow(row, sourceIndex)));
    return retVal;
}

/*!
    \internal
 */
bool QQmlSortFilterProxyModelPrivate::filterAcceptsColumnInternal(int row, const QModelIndex &sourceIndex) const
{
    Q_Q(const QQmlSortFilterProxyModel);
    return q->filterAcceptsColumn(row, sourceIndex);
}

/*!
    \internal
 */
QModelIndex QQmlSortFilterProxyModelPrivate::createIndex(int row, int column,
    QHash<QtPrivate::QModelIndexWrapper, QSortFilterProxyModelHelper::Mapping *>::const_iterator it) const {
    return q_func()->createIndex(row, column, *it);
}

/*!
    \internal
 */
void QQmlSortFilterProxyModelPrivate::changePersistentIndexList(const QModelIndexList &from, const QModelIndexList &to) {
    Q_Q(QQmlSortFilterProxyModel);
    q->changePersistentIndexList(from, to);
}

/*!
    \internal
 */
void QQmlSortFilterProxyModelPrivate::beginInsertRows(const QModelIndex &parent, int first, int last) {
    Q_Q(QQmlSortFilterProxyModel);
    q->beginInsertRows(parent, first, last);
}

/*!
    \internal
 */
void QQmlSortFilterProxyModelPrivate::beginInsertColumns(const QModelIndex &parent, int first, int last) {
    Q_Q(QQmlSortFilterProxyModel);
    q->beginInsertColumns(parent, first, last);
}

/*!
    \internal
 */
void QQmlSortFilterProxyModelPrivate::endInsertRows() {
    Q_Q(QQmlSortFilterProxyModel);
    q->endInsertRows();
}

/*!
    \internal
 */
void QQmlSortFilterProxyModelPrivate::endInsertColumns() {
    Q_Q(QQmlSortFilterProxyModel);
    q->endInsertColumns();
}

/*!
    \internal
 */
void QQmlSortFilterProxyModelPrivate::beginRemoveRows(const QModelIndex &parent, int first, int last) {
    Q_Q(QQmlSortFilterProxyModel);
    q->beginRemoveRows(parent, first, last);
}

/*!
    \internal
 */
void QQmlSortFilterProxyModelPrivate::beginRemoveColumns(const QModelIndex &parent, int first, int last) {
    Q_Q(QQmlSortFilterProxyModel);
    q->beginRemoveColumns(parent, first, last);
}

/*!
    \internal
 */
void QQmlSortFilterProxyModelPrivate::endRemoveRows() {
    Q_Q(QQmlSortFilterProxyModel);
    q->endRemoveRows();
}

/*!
    \internal
 */
void QQmlSortFilterProxyModelPrivate::endRemoveColumns() {
    Q_Q(QQmlSortFilterProxyModel);
    q->endRemoveColumns();
}

/*!
    \internal
 */
void QQmlSortFilterProxyModelPrivate::beginResetModel() {
    Q_Q(QQmlSortFilterProxyModel);
    q->beginResetModel();
}

/*!
    \internal
 */
void QQmlSortFilterProxyModelPrivate::endResetModel() {
    Q_Q(QQmlSortFilterProxyModel);
    q->endResetModel();
}

/*!
    \internal
 */
bool QQmlSortFilterProxyModelPrivate::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    return q_func()->filterAcceptsRow(source_row, source_parent);
}

/*!
    \internal
 */
bool QQmlSortFilterProxyModelPrivate::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const {
    return q_func()->filterAcceptsColumn(source_column, source_parent);
}

/*!
    \internal
 */
bool QQmlSortFilterProxyModelPrivate::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
    return q_func()->lessThan(source_left, source_right);
}

QT_END_NAMESPACE

#include "moc_qqmlsortfilterproxymodel_p.cpp"
