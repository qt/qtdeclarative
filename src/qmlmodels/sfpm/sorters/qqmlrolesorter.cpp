// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlrolesorter_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype RoleSorter
    \inherits Sorter
    \inqmlmodule QtQml.Models
    \since 6.10
    \preliminary
    \brief Sort data in a \l SortFilterProxyModel based on configured role
    name.

    RoleSorter allows the user to sort the data according to the role name
    as configured in the source model.

    The RoleSorter can be configured in the sort filter proxy model as below,

    \qml
    SortFilterProxyModel {
        sourceModel: model
        sorters: [
            RoleSorter { roleName: "firstname" }
        ]
    }
    \endqml
*/

QQmlRoleSorter::QQmlRoleSorter(QObject *parent) :
      QQmlSorterBase (new QQmlRoleSorterPrivate, parent)
{
}

QQmlRoleSorter::QQmlRoleSorter(QQmlSorterBasePrivate *priv, QObject *parent) :
      QQmlSorterBase (priv, parent)
{
}

/*!
    \qmlproperty string RoleSorter::roleName

    This property holds the role name that will be used to sort the data.

    The default value is display role.
*/
void QQmlRoleSorter::setRoleName(const QString& roleName)
{
    Q_D(QQmlRoleSorter);
    if (d->m_roleName == roleName)
        return;
    d->m_roleName = roleName;
    d->m_roleNameValidated = false;
    // Update the model for the change in the role name
    emit roleNameChanged();
    // Invalidate the model for the change in the role name
    invalidate();
}

const QString& QQmlRoleSorter::roleName() const
{
    Q_D(const QQmlRoleSorter);
    return d->m_roleName;
}

/*!
    \internal
*/
QPartialOrdering QQmlRoleSorter::compare(const QModelIndex& sourceLeft, const QModelIndex& sourceRight, const QQmlSortFilterProxyModel *proxyModel) const
{
    Q_D(const QQmlRoleSorter);
    int role = proxyModel->itemRoleForName(d->m_roleName);

    if (role < 0) {
        if (!d->m_roleName.isEmpty() && !d->m_roleNameValidated) {
            qWarning("Provided role name %s doesn't exist in the model", d->m_roleName.toUtf8().constData());
            d->m_roleNameValidated = true;
        }
        return QPartialOrdering::Unordered;
    }

    return QVariant::compare(proxyModel->sourceData(sourceLeft, role), proxyModel->sourceData(sourceRight, role));
}

void QQmlRoleSorter::update(const QQmlSortFilterProxyModel *)
{
    Q_D(QQmlRoleSorter);
    d->m_roleNameValidated = false;
}

QT_END_NAMESPACE

#include "moc_qqmlrolesorter_p.cpp"
