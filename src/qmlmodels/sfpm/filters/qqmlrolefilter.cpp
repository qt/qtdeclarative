// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlrolefilter_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype RoleFilter
    \inherits FilterBase
    \inqmlmodule QtQml.Models
    \since 6.10
    \preliminary
    \brief Abstract base type providing functionality to role-dependent filters.
*/

QQmlRoleFilter::QQmlRoleFilter(QObject *parent) :
      QQmlFilterBase (new QQmlRoleFilterPrivate, parent)
{
}

QQmlRoleFilter::QQmlRoleFilter(QQmlFilterBasePrivate *priv, QObject *parent) :
      QQmlFilterBase (priv, parent)
{

}

/*!
    \qmlproperty string RoleFilter::roleName

    This property holds the role name that will be used to filter the data.

    The default value is the display role.
*/
const QString& QQmlRoleFilter::roleName() const
{
    Q_D(const QQmlRoleFilter);
    return d->m_roleName;
}

void QQmlRoleFilter::setRoleName(const QString& roleName)
{
    Q_D(QQmlRoleFilter);
    if (d->m_roleName == roleName)
        return;
    d->m_roleName = roleName;
    d->m_roleNameValidated = false;
    emit roleNameChanged();
    // Invalidate the model
    invalidate();
}

/*!
    \internal
*/
int QQmlRoleFilter::itemRole(const QQmlSortFilterProxyModel *proxyModel) const
{
    Q_D(const QQmlRoleFilter);
    if (!d->m_roleName.isNull())
        return proxyModel->itemRoleForName(d->m_roleName);
    return -1;
}

void QQmlRoleFilter::update(const QQmlSortFilterProxyModel *)
{
    Q_D(QQmlRoleFilter);
    d->m_roleNameValidated = false;
}

QT_END_NAMESPACE

#include "moc_qqmlrolefilter_p.cpp"
