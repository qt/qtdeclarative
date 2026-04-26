// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlalloffilter_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype AllOfFilter
    \inherits FilterBase
    \inqmlmodule QtQml.Models
    \since 6.12
    \preliminary
    \brief Combines multiple filters using logical AND in a
    \l SortFilterProxyModel.

    AllOfFilter groups a set of child filters and finds a matching row or
    column only when all child filters accept it. It is equivalent to applying
    multiple filters directly to \l SortFilterProxyModel, but allows the
    group itself to be enabled, disabled, or inverted as a single unit.

    The following snippet shows how AllOfFilter can be used to include
    only rows where \c status is \c "active" and \c favorite is \c true:

    \qml
    SortFilterProxyModel {
        sourceModel: model
        filters: [
            AllOfFilter {
                ValueFilter {
                    roleName: "parent";
                    value: "root"
                }
                ValueFilter {
                    roleName: "status";
                    value: "active"
                }
            }
        ]
    }
    \endqml
*/

/*!
    \qmlproperty list<Filter> AllOfFilter::filters

    The list of child filters evaluated with logical AND. A row is accepted
    if all the filters in this list accepts it.

    This is the default property, so filters can be declared as direct
    children without the \c filters keyword:

    \qml
    AllOfFilter {
        ValueFilter { roleName: "status"; value: "active" }
        ValueFilter { roleName: "parent"; value: "root" }
    }
    \endqml
*/

QQmlAllOfFilter::QQmlAllOfFilter(QObject *parent) :
      QQmlCompositeFilterBase(new QQmlAllOfFilterPrivate, parent)
{

}

/*!
    \internal
*/
bool QQmlAllOfFilter::filterAcceptsRowInternal(int row, const QModelIndex& sourceParent, const QQmlSortFilterProxyModel *proxyModel) const
{
    // Note: QQmlFilterCompositor evaluates rows using std::all_of (AND logic),
    // which matches AllOf semantics exactly, so no override needed.
    return QQmlCompositeFilterBase::filterAcceptsRowInternal(row, sourceParent, proxyModel);
}

bool QQmlAllOfFilter::filterAcceptsColumnInternal(int column, const QModelIndex& sourceParent, const QQmlSortFilterProxyModel *proxyModel) const
{
    // Note: QQmlFilterCompositor evaluates rows using std::all_of (AND logic),
    // which matches AllOf semantics exactly, so no override needed.
    return QQmlCompositeFilterBase::filterAcceptsColumnInternal(column, sourceParent, proxyModel);
}

QT_END_NAMESPACE

#include "moc_qqmlalloffilter_p.cpp"
