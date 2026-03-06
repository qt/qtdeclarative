// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlvaluefilter_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ValueFilter
    \inherits RoleFilter
    \inqmlmodule QtQml.Models
    \since 6.10
    \preliminary
    \brief Filters data in a \l SortFilterProxyModel based on role name and
    value.

    ValueFilter allows the user to filter the data according to the role name
    or specified value or both as configured in the source model. The role name
    used to filter the data shall be based on model
    \l{QAbstractItemModel::roleNames()}{role name}. The default value for
    role name is \c "display".

    The following snippet shows how ValueFilter can be used to only include
    data from the source model where the value of the role name \c "favorite"
    is \c "true":

    \qml
    SortFilterProxyModel {
        sourceModel: model
        filters: [
            ValueFilter {
                roleName: "favorite"
                value: true
            }
        ]
    }
    \endqml
*/

QQmlValueFilter::QQmlValueFilter(QObject *parent) :
      QQmlRoleFilter (new QQmlValueFilterPrivate, parent)
{

}

/*!
    \qmlproperty variant ValueFilter::value

    This property holds the specific value that can be used to filter the data.
*/
const QVariant& QQmlValueFilter::value() const
{
    Q_D(const QQmlValueFilter);
    return d->m_value;
}

void QQmlValueFilter::setValue(const QVariant& value)
{
    Q_D(QQmlValueFilter);
    if (d->m_value == value)
        return;
    d->m_value = value;
    // Update the model for the change in the role name
    emit valueChanged();
    // Invalidate the model for the change in the role name
    invalidate();
}

void QQmlValueFilter::resetValue()
{
    Q_D(QQmlValueFilter);
    d->m_value = QVariant();
}

/*!
    \internal
*/
bool QQmlValueFilter::filterAcceptsRowInternal(int row, const QModelIndex& sourceParent, const QQmlSortFilterProxyModel *proxyModel) const
{
    Q_D(const QQmlValueFilter);
    if (d->m_roleName.isEmpty())
        return true;
    int role = itemRole(proxyModel);
    if (role < 0) {
        if (!d->m_roleName.isEmpty() && !d->m_roleNameValidated) {
            qWarning("Provided role name %s doesn't exist in the model", d->m_roleName.toUtf8().constData());
            d->m_roleNameValidated = true;
        }
        return false;
    }
    const bool isValidVal = (!d->m_value.isValid() || !d->m_value.isNull());
    if (column() > -1) {
        const QModelIndex &index = proxyModel->sourceModel()->index(row, column(), sourceParent);
        const QVariant &value =  proxyModel->sourceModel()->data(index, role);
        return (value.isValid() && (!isValidVal || d->m_value == value));
    } else {
        const int columnCount = proxyModel->sourceModel()->columnCount(sourceParent);
        for (int column = 0; column < columnCount; column++) {
            const QModelIndex &index = proxyModel->sourceModel()->index(row, column, sourceParent);
            const QVariant &value =  proxyModel->sourceModel()->data(index, role);
            if (value.isValid() && (!isValidVal || d->m_value == value))
                return true;
        }
    }
    return false;
}

QT_END_NAMESPACE

#include "moc_qqmlvaluefilter_p.cpp"
