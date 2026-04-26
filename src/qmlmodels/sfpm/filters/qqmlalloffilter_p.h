// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLALLOFFILTER_P_H
#define QQMLALLOFFILTER_P_H

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

#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>
#include <QtQmlModels/private/qqmlcompositefilterbase_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModel;
class QQmlAllOfFilterPrivate;

class Q_QMLMODELS_EXPORT QQmlAllOfFilter : public QQmlCompositeFilterBase
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AllOfFilter)
    QML_ADDED_IN_VERSION(6, 12)

public:
    explicit QQmlAllOfFilter(QObject *parent = nullptr);

    bool filterAcceptsRowInternal(int, const QModelIndex&, const QQmlSortFilterProxyModel *) const override;
    bool filterAcceptsColumnInternal(int, const QModelIndex&, const QQmlSortFilterProxyModel *) const override;

private:
    Q_DECLARE_PRIVATE(QQmlAllOfFilter)
};

class QQmlAllOfFilterPrivate : public QQmlCompositeFilterBasePrivate
{
    Q_DECLARE_PUBLIC(QQmlAllOfFilter)
};

QT_END_NAMESPACE

#endif // QQMLALLOFFILTER_P_H
