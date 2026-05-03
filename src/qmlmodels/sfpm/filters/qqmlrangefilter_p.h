// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLRANGEFILTER_P_H
#define QQMLRANGEFILTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQmlModels/private/qqmlrolefilter_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModel;
class QQmlRangeFilterPrivate;

class Q_QMLMODELS_EXPORT QQmlRangeFilter : public QQmlRoleFilter
{
    Q_OBJECT
    Q_PROPERTY(QVariant minimum READ minimum WRITE setMinimum RESET resetMinimum NOTIFY minimumChanged)
    Q_PROPERTY(QVariant maximum READ maximum WRITE setMaximum RESET resetMaximum NOTIFY maximumChanged)
    QML_NAMED_ELEMENT(RangeFilter)
    QML_ADDED_IN_VERSION(6, 12)

public:
    struct RangeExclusiveBoundary
    {
        QVariant value;
    };

    explicit QQmlRangeFilter(QObject *parent = nullptr);
    ~QQmlRangeFilter() = default;

    const QVariant &minimum() const;
    void setMinimum(const QVariant &minimum);
    void resetMinimum();

    const QVariant &maximum() const;
    void setMaximum(const QVariant &maximum);
    void resetMaximum();

    Q_INVOKABLE static QVariant exclusive(const QVariant &value);

    bool filterAcceptsRowInternal(int row, const QModelIndex &sourceParent,
                                  const QQmlSortFilterProxyModel *proxyModel) const override;

Q_SIGNALS:
    void minimumChanged();
    void maximumChanged();

private:
    Q_DECLARE_PRIVATE(QQmlRangeFilter)
};

class QQmlRangeFilterPrivate : public QQmlRoleFilterPrivate
{
    Q_DECLARE_PUBLIC(QQmlRangeFilter)

public:
    QVariant m_minimum;
    QVariant m_maximum;
    bool m_minimumInclusive = true;
    bool m_maximumInclusive = true;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlRangeFilter::RangeExclusiveBoundary)

#endif // QQMLRANGEFILTER_P_H
