// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlstringsorter_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype StringSorter
    \inherits Sorter
    \inqmlmodule QtQml.Models
    \since 6.10
    \preliminary
    \brief  Sort data in a \l SortFilterProxyModel based on ordering of the
    locale.

    StringSorter allows the user to sort the data according to the role name
    as configured in the source model. StringSorter compares strings according
    to a localized collation algorithm.

    The StringSorter can be configured in the sort filter proxy model as below,

    \qml
    SortFilterProxyModel {
        sourceModel: model
        sorters: [
            StringSorter { roleName: "name" }
        ]
    }
    \endqml
*/

QQmlStringSorter::QQmlStringSorter(QObject *parent) :
      QQmlRoleSorter (new QQmlStringSorterPrivate, parent)
{
}

/*!
    \qmlproperty Qt::CaseSensitivity StringSorter::caseSensitivity

    This property holds the case sensitivity of the sorter.

    The default value is Qt::CaseSensitive.
*/
Qt::CaseSensitivity QQmlStringSorter::caseSensitivity() const
{
    Q_D(const QQmlStringSorter);
    return d->m_collator.caseSensitivity();
}

void QQmlStringSorter::setCaseSensitivity(Qt::CaseSensitivity caseSensitivity)
{
    Q_D(QQmlStringSorter);
    if (d->m_collator.caseSensitivity() == caseSensitivity)
        return;
    d->m_collator.setCaseSensitivity(caseSensitivity);
    emit caseSensitivityChanged();
    invalidate();
}

/*!
    \qmlproperty bool StringSorter::ignorePunctuation

    This property holds whether the sorter ignores punctation.
    If \c ignorePunctuation is \c true, punctuation characters and symbols are
    ignored when determining sort order.

    The default value is \c false.
*/
bool QQmlStringSorter::ignorePunctuation() const
{
    Q_D(const QQmlStringSorter);
    return d->m_collator.ignorePunctuation();
}

void QQmlStringSorter::setIgnorePunctuation(bool ignorePunctuation)
{
    Q_D(QQmlStringSorter);
    if (d->m_collator.ignorePunctuation() == ignorePunctuation)
        return;
    d->m_collator.setIgnorePunctuation(ignorePunctuation);
    emit ignorePunctuationChanged();
    invalidate();
}

/*!
    \qmlproperty Locale StringSorter::locale

    This property holds the locale of the sorter.

    The default value is \l QLocale::system()
*/
QLocale QQmlStringSorter::locale() const
{
    Q_D(const QQmlStringSorter);
    return d->m_collator.locale();
}

void QQmlStringSorter::setLocale(const QLocale &locale)
{
    Q_D(QQmlStringSorter);
    if (d->m_collator.locale() == locale)
        return;
    d->m_collator.setLocale(locale);
    emit localeChanged();
    invalidate();
}

/*!
    \qmlproperty bool StringSorter::numericMode

    This property holds whether the numeric mode of the sorter is enabled.

    The default value is \c false.
*/
bool QQmlStringSorter::numericMode() const
{
    Q_D(const QQmlStringSorter);
    return d->m_collator.numericMode();
}

void QQmlStringSorter::setNumericMode(bool numericMode)
{
    Q_D(QQmlStringSorter);
    if (d->m_collator.numericMode() == numericMode)
        return;

    d->m_collator.setNumericMode(numericMode);
    emit numericModeChanged();
    invalidate();
}
/*!
    \internal
*/
QPartialOrdering QQmlStringSorter::compare(const QModelIndex &sourceLeft, const QModelIndex &sourceRight, const QQmlSortFilterProxyModel* proxyModel) const
{
    Q_D(const QQmlStringSorter);
    if (int role = proxyModel->itemRoleForName(d->m_roleName); role > -1) {
        const QVariant first = proxyModel->sourceData(sourceLeft, role);
        const QVariant second = proxyModel->sourceData(sourceRight, role);
        const int result = d->m_collator.compare(first.toString(), second.toString());
        return (result <= 0) ? ((result < 0) ? QPartialOrdering::Less : QPartialOrdering::Equivalent) : QPartialOrdering::Greater;
    }
    return QPartialOrdering::Unordered;
}

QT_END_NAMESPACE

#include "moc_qqmlstringsorter_p.cpp"
