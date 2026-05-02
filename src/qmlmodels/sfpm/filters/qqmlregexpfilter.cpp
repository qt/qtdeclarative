// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQml/qqmlinfo.h>
#include <QtQmlModels/private/qqmlregexpfilter_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \qmltype RegExpFilter
    \inherits RoleFilter
    \inqmlmodule QtQml.Models
    \since 6.12
    \preliminary
    \brief Filters data in a \l SortFilterProxyModel by matching a role value
    against a regular expression.

    RegExpFilter allows filtering model data using regular expression
    matching on a specified role.

    A JavaScript regular expression literal can be assigned directly to
    \l regExp:

    \qml
    SortFilterProxyModel {
        sourceModel: processModel
        filters: RegExpFilter {
            roleName: "userId"
            regExp: /root/i
        }
    }
    \endqml
*/

QQmlRegExpFilter::QQmlRegExpFilter(QObject *parent)
    : QQmlRoleFilter(new QQmlRegExpFilterPrivate, parent)
{
}

/*!
    \qmlproperty regexp RegExpFilter::regExp

    This property holds the regular expression used to filter the data.
    A JavaScript regular expression literal can be assigned directly.

    When the \l regExp has an empty pattern, the filter is inactive and
    all rows pass through.
*/
QRegularExpression QQmlRegExpFilter::regExp() const
{
    Q_D(const QQmlRegExpFilter);
    return d->regExp;
}

void QQmlRegExpFilter::setRegExp(const QRegularExpression &regExp)
{
    Q_D(QQmlRegExpFilter);
    if (d->regExp == regExp)
        return;
    d->regExp = regExp;
    emit regExpChanged();
    invalidate();
}

/*!
    \internal
*/
bool QQmlRegExpFilter::filterAcceptsRowInternal(int row, const QModelIndex &sourceParent,
                                                const QQmlSortFilterProxyModel *proxyModel) const
{
    Q_D(const QQmlRegExpFilter);
    if (d->m_roleName.isEmpty() || !d->regExp.isValid())
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

    auto filterData = [&](const QModelIndex &index) -> bool {
        const QString value = proxyModel->sourceModel()->data(index, role).toString();
        // If the value is empty and there is a regular expresison to check, filter the row
        if (value.isEmpty())
            return false;
        const QRegularExpressionMatch match =
            d->regExp.match(value, 0, QRegularExpression::NormalMatch);
        return match.hasMatch();
    };

    if (column() > -1) {
        const QModelIndex index = proxyModel->sourceModel()->index(row, column(), sourceParent);
        return filterData(index);
    } else {
        const int columnCount = proxyModel->sourceModel()->columnCount(sourceParent);
        for (int column = 0; column < columnCount; ++column) {
            if (filterData(proxyModel->sourceModel()->index(row, column, sourceParent)))
                return true;
        }
    }

    return false;
}

QT_END_NAMESPACE

#include "moc_qqmlregexpfilter_p.cpp"

