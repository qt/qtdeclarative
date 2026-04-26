// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>
#include <QtQmlModels/private/qqmlfilterbase_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Filter
    \inherits QtObject
    \inqmlmodule QtQml.Models
    \since 6.10
    \preliminary
    \brief Abstract base type providing functionality common to filters.

    Filter provides a set of common properties for all the filters that they
    inherit from.
*/

QQmlFilterBase::QQmlFilterBase(QQmlFilterBasePrivate *privObj, QObject *parent)
    : QObject (*privObj, parent)
{
}

/*!
    \qmlproperty bool Filter::enabled

    This property enables the \l SortFilterProxyModel to consider this filter
    while filtering the model data.

    The default value is \c true.
*/
bool QQmlFilterBase::enabled() const
{
    Q_D(const QQmlFilterBase);
    return d->m_enabled;
}

void QQmlFilterBase::setEnabled(bool enabled)
{
    Q_D(QQmlFilterBase);
    if (d->m_enabled == enabled)
        return;
    d->m_enabled = enabled;
    invalidate(true);
    emit enabledChanged();
}

/*!
    \qmlproperty bool Filter::inverted

    This property inverts the filter, causing the data that would normally be
    filtered out to be presented instead.

    The default value is \c false.
*/
bool QQmlFilterBase::isInverted() const
{
    Q_D(const QQmlFilterBase);
    return d->m_inverted;
}

void QQmlFilterBase::setInverted(bool invert)
{
    Q_D(QQmlFilterBase);
    if (d->m_inverted == invert)
        return;
    d->m_inverted = invert;
    invalidate();
    emit invertedChanged();
}

/*!
    \qmlproperty int Filter::column

    This property specifies which column in the model the filter should be
    applied on. If the value is \c -1, the filter will be applied to all
    the columns in the model.

    The default value is \c -1.
*/
int QQmlFilterBase::column() const
{
    Q_D(const QQmlFilterBase);
    return d->m_filterColumn;
}

void QQmlFilterBase::setColumn(int column)
{
    Q_D(QQmlFilterBase);
    if (d->m_filterColumn == column)
        return;
    d->m_filterColumn = column;
    invalidate();
    emit columnChanged();
}

/*!
    \internal
*/
void QQmlFilterBase::invalidate(bool updateCache)
{
    // Update the cached filters and invalidate the model
    if (updateCache)
        emit invalidateCache(this);
    emit invalidateModel();
}

QT_END_NAMESPACE

#include "moc_qqmlfilterbase_p.cpp"
