// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlanyoffilter_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype AnyOfFilter
    \inherits FilterBase
    \inqmlmodule QtQml.Models
    \since 6.12
    \preliminary
    \brief Combines multiple filters using logical OR in a
    \l SortFilterProxyModel.

    AnyOfFilter groups a set of child filters and finds a matching row or column
    for least one of child filter in the group. This is useful when you have many
    independent conditions to include a row, without requiring all conditions to
    be satisfied.

    The following snippet shows how AnyOfFilter can be used to include
    rows where \c status is either \c "active" or \c "pending":

    \qml
    SortFilterProxyModel {
        sourceModel: model
        filters: [
            AnyOfFilter {
                ValueFilter {
                    roleName: "status"
                    value: "active"
                }
                ValueFilter {
                    roleName: "status"
                    value: "pending"
                }
            }
        ]
    }
    \endqml
*/

/*!
    \qmlproperty list<FilterBase> AnyOfFilter::filters

    The list of child filters evaluated with logical OR. A row is accepted
    if at least one filter in this list accepts it.

    This is the default property, so filters can be declared as direct
    children without the \c filters keyword:

    \qml
    AnyOfFilter {
        ValueFilter { roleName: "status"; value: "active" }
        ValueFilter { roleName: "status"; value: "pending" }
    }
    \endqml
*/

QQmlAnyOfFilter::QQmlAnyOfFilter(QObject *parent) :
      QQmlCompositeFilterBase(new QQmlAnyOfFilterPrivate, parent)
{

}

/*!
    \internal
*/
bool QQmlAnyOfFilter::filterAcceptsRowInternal(int row, const QModelIndex& sourceParent, const QQmlSortFilterProxyModel *proxyModel) const
{
    Q_D(const QQmlAnyOfFilter);
    const auto &filters = d->m_effectiveFilters;
    return std::any_of(filters.begin(), filters.end(),
            [row, &sourceParent, proxyModel](const QQmlFilterBase *filter) {
                const bool filterStatus = filter->filterAcceptsRowInternal(row, sourceParent, proxyModel);
                return filter->isInverted() != filterStatus;
            });
}

bool QQmlAnyOfFilter::filterAcceptsColumnInternal(int column, const QModelIndex& sourceParent, const QQmlSortFilterProxyModel *proxyModel) const
{
    Q_D(const QQmlAnyOfFilter);
    const auto &filters = d->m_effectiveFilters;
    return std::any_of(filters.begin(), filters.end(),
            [column, &sourceParent, proxyModel](const QQmlFilterBase *filter) {
                if (!filter->supportColumnFiltering())
                    return true;
                const bool filterStatus = filter->filterAcceptsColumnInternal(column, sourceParent, proxyModel);
                return filter->isInverted() != filterStatus;
            });
}

QT_END_NAMESPACE

#include "moc_qqmlanyoffilter_p.cpp"
