// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlfiltercompositor_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY (lcSfpmFilterCompositor, "qt.qml.sfpmfiltercompositor")

QQmlFilterCompositor::QQmlFilterCompositor(QObject *parent)
    : QQmlFilterBase(new QQmlFilterCompositorPrivate, parent)
{
    Q_D(QQmlFilterCompositor);
    d->init();
    // Connect the model reset with the update in the filter
    // The cache need to be updated once the model is reset with the
    // source model data. This is because, there are chances that
    // the filter can be enabled or disabled in effective filter list
    // such as the configured role name in the filter doesn't match
    // with any role name in the model
    if (d->m_sfpmModel)
        connect(d->m_sfpmModel, &QQmlSortFilterProxyModel::modelReset,
                this, &QQmlFilterCompositor::updateFilters);
}

QQmlFilterCompositor::QQmlFilterCompositor(QQmlFilterBasePrivate *priv, QObject *parent)
    : QQmlFilterBase(priv, parent)
{

}

void QQmlFilterCompositorPrivate::init()
{
    Q_Q(QQmlFilterCompositor);
    m_sfpmModel = qobject_cast<QQmlSortFilterProxyModel *>(q->parent());
}

void QQmlFilterCompositor::append(QQmlListProperty<QQmlFilterBase> *filterComp, QQmlFilterBase* filter)
{
    auto *filterCompositor = reinterpret_cast<QQmlFilterCompositor *> (filterComp->object);
    filterCompositor->append(filter);
}

qsizetype QQmlFilterCompositor::count(QQmlListProperty<QQmlFilterBase> *filterComp)
{
    auto *filterCompositor = reinterpret_cast<QQmlFilterCompositor *> (filterComp->object);
    return filterCompositor->count();
}

QQmlFilterBase *QQmlFilterCompositor::at(QQmlListProperty<QQmlFilterBase> *filterComp, qsizetype index)
{
    auto *filterCompositor = reinterpret_cast<QQmlFilterCompositor *> (filterComp->object);
    return filterCompositor->at(index);
}

void QQmlFilterCompositor::clear(QQmlListProperty<QQmlFilterBase> *filterComp)
{
    auto *filterCompositor = reinterpret_cast<QQmlFilterCompositor *> (filterComp->object);
    filterCompositor->clear();
}

void QQmlFilterCompositor::append(QQmlFilterBase *filter)
{
    if (!filter)
        return;

    Q_D(QQmlFilterCompositor);
    d->m_filters.append(filter);
    // This is needed as its required to update cache when there is any
    // change in the filter itself (for instance, a change in the priority of
    // the filter)
    QObject::connect(filter, &QQmlFilterBase::invalidateCache,
                     this, &QQmlFilterCompositor::refreshCache);

    if (d->m_sfpmModel) {
        // Connect the filter to the corresponding slot to invalidate the model
        // and the filter cache
        QObject::connect(filter, &QQmlFilterBase::invalidateModel,
                         d->m_sfpmModel, &QQmlSortFilterProxyModel::invalidate);
        // Validate the filter for any precondition which can be compared with
        // sfpm and update the filter cache accordingly
        filter->update(d->m_sfpmModel);
        refreshCache();
        // Since we added new filter to the list, emit the filter changed signal
        // for the filters that have been appended to the list
        emit d->m_sfpmModel->filtersChanged();
    }
}

qsizetype QQmlFilterCompositor::count()
{
    Q_D(QQmlFilterCompositor);
    return d->m_filters.count();
}

QQmlFilterBase *QQmlFilterCompositor::at(qsizetype index)
{
    Q_D(QQmlFilterCompositor);
    return d->m_filters.at(index);
}

void QQmlFilterCompositor::clear()
{
    Q_D(QQmlFilterCompositor);
    d->m_effectiveFilters.clear();
    d->m_filters.clear();
    // Emit the filter changed signal as we cleared the filter list
    if (d->m_sfpmModel)
        emit d->m_sfpmModel->filtersChanged();
}

QList<QQmlFilterBase *> QQmlFilterCompositor::filters()
{
    Q_D(QQmlFilterCompositor);
    return d->m_filters;
}

QQmlListProperty<QQmlFilterBase> QQmlFilterCompositor::filtersListProperty()
{
    Q_D(QQmlFilterCompositor);
    return QQmlListProperty<QQmlFilterBase>(reinterpret_cast<QObject*>(this), &d->m_filters,
                                                 QQmlFilterCompositor::append,
                                                 QQmlFilterCompositor::count,
                                                 QQmlFilterCompositor::at,
                                                 QQmlFilterCompositor::clear);
}

void QQmlFilterCompositor::updateFilters()
{
    Q_D(QQmlFilterCompositor);
    // Update filters that have dependencies with the model data
    for (auto &filter: d->m_filters)
        filter->update(d->m_sfpmModel);
    // Update the cache
    refreshCache();
}

void QQmlFilterCompositor::refreshCache()
{
    Q_D(QQmlFilterCompositor);
    // Clear the existing cache
    d->m_effectiveFilters.clear();
    if (d->m_sfpmModel && d->m_sfpmModel->sourceModel()) {
        for (auto &filter: d->m_filters) {
            if (filter->isActive())
                d->m_effectiveFilters.append(filter);
        }
    }
}

bool QQmlFilterCompositor::filterAcceptsRowInternal(int row, const QModelIndex& sourceParent, const QQmlSortFilterProxyModel *proxyModel) const
{
    Q_D(const QQmlFilterCompositor);
    // Check the data against the configured filters and if nothing configured,
    // dont filter the data
    return std::all_of(d->m_effectiveFilters.begin(), d->m_effectiveFilters.end(),
            [row, &sourceParent, proxyModel](const QQmlFilterBase *filter) {
                const bool filterStatus = filter->filterAcceptsRowInternal(row, sourceParent, proxyModel);
                return filter->isInverted() != filterStatus;
            });
}

bool QQmlFilterCompositor::filterAcceptsColumnInternal(int column, const QModelIndex& sourceParent, const QQmlSortFilterProxyModel *proxyModel) const
{
    Q_D(const QQmlFilterCompositor);
    // Check the data against the configured filters and if nothing configured,
    // dont filter the data
    return std::all_of(d->m_effectiveFilters.begin(), d->m_effectiveFilters.end(),
            [column, &sourceParent, proxyModel](const QQmlFilterBase *filter) {
                if (!filter->supportColumnFiltering())
                    return true;
                const bool filterStatus = filter->filterAcceptsColumnInternal(column, sourceParent, proxyModel);
                return filter->isInverted() != filterStatus;
            });
}

QT_END_NAMESPACE

#include "moc_qqmlfiltercompositor_p.cpp"
