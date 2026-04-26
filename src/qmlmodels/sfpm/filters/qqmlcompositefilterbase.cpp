// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlcompositefilterbase_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype CompositeFilter
    \internal
    \inherits FilterBase
    \inqmlmodule QtQml.Models
    \since 6.12
    \preliminary
    \brief Abstract base type providing functionality common to composite
    filters.

    CompositeFilter provides a set of common properties that all composite
    filter types inherit, such as \l AllOfFilter and \l AnyOfFilter.
*/

QQmlCompositeFilterBase::QQmlCompositeFilterBase(QObject *parent)
    : QQmlFilterCompositor(new QQmlFilterCompositorPrivate, parent)
{
}

QQmlCompositeFilterBase::QQmlCompositeFilterBase(QQmlFilterCompositorPrivate *priv, QObject *parent)
    : QQmlFilterCompositor(priv, parent)
{

}

void QQmlCompositeFilterBase::update(const QQmlSortFilterProxyModel *proxyModel)
{
    Q_D(QQmlCompositeFilterBase);
    auto *sfpm = const_cast<QQmlSortFilterProxyModel *>(proxyModel);

    if (d->m_sfpmModel != sfpm) {
        if (d->m_sfpmModel) {
            disconnect(d->m_sfpmModel, &QQmlSortFilterProxyModel::modelReset,
                       this, &QQmlFilterCompositor::updateFilters);
            for (auto &filter: d->m_filters)
                disconnect(filter, &QQmlFilterBase::invalidateModel,
                           d->m_sfpmModel, &QQmlSortFilterProxyModel::invalidate);
        }

        d->m_sfpmModel = sfpm;

        if (d->m_sfpmModel) {
            connect(d->m_sfpmModel, &QQmlSortFilterProxyModel::modelReset,
                    this, &QQmlFilterCompositor::updateFilters);
            for (auto &filter: d->m_filters)
                connect(filter, &QQmlFilterBase::invalidateModel,
                        d->m_sfpmModel, &QQmlSortFilterProxyModel::invalidate);
        }
    }

    updateFilters();
}

bool QQmlCompositeFilterBase::isActive() const
{
    Q_D(const QQmlFilterCompositor);
    return enabled() && !d->m_effectiveFilters.isEmpty();
}

void QQmlCompositeFilterBase::refreshCache()
{
    Q_D(QQmlFilterCompositor);
    const auto prevCount = d->m_effectiveFilters.count();
    QQmlFilterCompositor::refreshCache();
    if (d->m_effectiveFilters.count() != prevCount)
        emit invalidateCache(this);
}

bool QQmlCompositeFilterBase::supportColumnFiltering() const
{
    Q_D(const QQmlFilterCompositor);
    return std::any_of(d->m_effectiveFilters.begin(), d->m_effectiveFilters.end(),
                [](const QQmlFilterBase *f) { return f->supportColumnFiltering(); });
}

QT_END_NAMESPACE

#include "moc_qqmlcompositefilterbase_p.cpp"
