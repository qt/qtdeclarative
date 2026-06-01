// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlrangefilter_p.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype RangeFilter
    \inherits RoleFilter
    \inqmlmodule QtQml.Models
    \since 6.12
    \preliminary
    \brief Filters data in a \l SortFilterProxyModel by testing whether a role
    value falls within a given range.

    RangeFilter accepts rows where the value of the configured role falls
    between \l minimum and \l maximum. Both bounds are inclusive by default.

    The following example shows processes with the CPU usage in the range
    between 70 and 100:

    \qml
    SortFilterProxyModel {
        sourceModel: processModel
        filters: [
            RangeFilter {
                roleName: "cpuUsage"
                minimum: 70
                maximum: 100
            }
        ]
    }
    \endqml

    Use \l exclusive() when a bound should exclude the boundary value itself:

    \qml
    RangeFilter {
        roleName: "cpuUsage"
        minimum: exclusive(70)
        maximum: 100
    }
    \endqml
*/

QQmlRangeFilter::QQmlRangeFilter(QObject *parent)
    : QQmlRoleFilter(new QQmlRangeFilterPrivate, parent)
{
}

/*!
    \qmlproperty variant RangeFilter::minimum

    The lower bound of the range. Rows whose role value is less than this
    value are excluded. When not set, there is no lower bound.

    Assigning a plain value makes the bound inclusive — rows equal to
    \c minimum are accepted. Use \l exclusive() to exclude the boundary
    value itself.
*/
const QVariant &QQmlRangeFilter::minimum() const
{
    Q_D(const QQmlRangeFilter);
    return d->m_minimum;
}

void QQmlRangeFilter::setMinimum(const QVariant &minimum)
{
    Q_D(QQmlRangeFilter);

    QVariant minValue = minimum;
    bool minInclusive = true;
    if (minimum.metaType() == QMetaType::fromType<QQmlRangeFilter::RangeExclusiveBoundary>()) {
        minValue = minimum.value<QQmlRangeFilter::RangeExclusiveBoundary>().value;
        minInclusive = false;
    }
    if (d->m_minimum == minValue && d->m_minimumInclusive == minInclusive)
        return;
    d->m_minimum = minValue;
    d->m_minimumInclusive = minInclusive;
    emit minimumChanged();
    invalidate();
}

void QQmlRangeFilter::resetMinimum()
{
    Q_D(QQmlRangeFilter);
    if (!d->m_minimum.isValid())
        return;
    d->m_minimum = QVariant();
    d->m_minimumInclusive = true;
    emit minimumChanged();
    invalidate();
}

/*!
    \qmlproperty variant RangeFilter::maximum

    The upper bound of the range. Rows whose role value is greater than this
    value are excluded. When not set, there is no upper bound.

    Assigning a plain value makes the bound inclusive — rows equal to
    \c maximum are accepted. Use \l exclusive() to exclude the boundary
    value itself.
*/
const QVariant &QQmlRangeFilter::maximum() const
{
    Q_D(const QQmlRangeFilter);
    return d->m_maximum;
}

void QQmlRangeFilter::setMaximum(const QVariant &maximum)
{
    Q_D(QQmlRangeFilter);
    QVariant maxValue = maximum;
    bool maxInclusive = true;
    if (maximum.metaType() == QMetaType::fromType<QQmlRangeFilter::RangeExclusiveBoundary>()) {
        maxValue = maximum.value<QQmlRangeFilter::RangeExclusiveBoundary>().value;
        maxInclusive = false;
    }
    d->m_maximum = maxValue;
    d->m_maximumInclusive = maxInclusive;
    emit maximumChanged();
    invalidate();
}

void QQmlRangeFilter::resetMaximum()
{
    Q_D(QQmlRangeFilter);
    if (!d->m_maximum.isValid())
        return;
    d->m_maximum = QVariant();
    d->m_maximumInclusive = true;
    emit maximumChanged();
    invalidate();
}

/*!
    \qmlmethod variant RangeFilter::exclusive(value)

    Returns a boundary that excludes \a value from the range.
    Rows whose role value equals \a value are rejected.

    \qml
    RangeFilter {
        roleName: "score"
        minimum: 0
        maximum: RangeFilter.exclusive(100)
    \endqml
*/
QVariant QQmlRangeFilter::exclusive(const QVariant &value)
{
    return QVariant::fromValue(QQmlRangeFilter::RangeExclusiveBoundary{value});
}

/*!
    \internal
*/
bool QQmlRangeFilter::filterAcceptsRowInternal(int row, const QModelIndex &sourceParent,
                                               const QQmlSortFilterProxyModel *proxyModel) const
{
    Q_D(const QQmlRangeFilter);
    if (d->m_roleName.isEmpty())
        return true;

    const int role = itemRole(proxyModel);
    if (role < 0) {
        if (!d->m_roleNameValidated) {
            qWarning("Provided role name %s doesn't exist in the model",
                     d->m_roleName.toUtf8().constData());
            d->m_roleNameValidated = true;
        }
        return false;
    }

    const bool isMinimumSet = d->m_minimum.isValid() && !d->m_minimum.isNull();
    const bool isMaximumSet = d->m_maximum.isValid() && !d->m_maximum.isNull();

    if (!isMinimumSet && !isMaximumSet)
        return true;

    auto filterData = [&](const QVariant &value) -> bool {
        if (!value.isValid())
            return false;
        auto *pModel = const_cast<QQmlSortFilterProxyModel *>(proxyModel);
        if (isMinimumSet) {
            const auto order = pModel->compareData(value, d->m_minimum);
            if (d->m_minimumInclusive ? order == Qt::weak_ordering::less :
                        order != Qt::weak_ordering::greater)
                return false;
        }
        if (isMaximumSet) {
            const auto order = pModel->compareData(value, d->m_maximum);
            if (d->m_maximumInclusive ? order == Qt::weak_ordering::greater :
                        order != Qt::weak_ordering::less)
                return false;
        }
        return true;
    };

    if (column() > -1) {
        const QModelIndex &index = proxyModel->sourceModel()->index(row, column(), sourceParent);
        return filterData(proxyModel->sourceModel()->data(index, role));
    } else {
        const int columnCount = proxyModel->sourceModel()->columnCount(sourceParent);
        for (int col = 0; col < columnCount; ++col) {
             const QModelIndex &index = proxyModel->sourceModel()->index(row, col, sourceParent);
             if (filterData(proxyModel->sourceModel()->data(index, role)))
                return true;
        }
    }

    return false;
}

QT_END_NAMESPACE

#include "moc_qqmlrangefilter_p.cpp"
