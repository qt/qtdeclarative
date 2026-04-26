// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLCOMPOSITEFILTERBASE_P_H
#define QQMLCOMPOSITEFILTERBASE_P_H

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

#include <QtQml/qqmlinfo.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>
#include <QtQmlModels/private/qqmlfiltercompositor_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModel;
class QQmlCompositeFilterBasePrivate;

class Q_QMLMODELS_EXPORT QQmlCompositeFilterBase: public QQmlFilterCompositor
{
    Q_OBJECT
    Q_CLASSINFO("DefaultProperty", "filters")
    Q_PROPERTY(QQmlListProperty<QQmlFilterBase> filters READ filtersListProperty CONSTANT FINAL)
    QML_NAMED_ELEMENT(CompositeFilterBase)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(6, 12)

public:
    explicit QQmlCompositeFilterBase(QObject *parent);
    void update(const QQmlSortFilterProxyModel *model) override;
    bool isActive() const override;

    int column() const override {
        qmlWarning(this) << "column is not supported on composite filters; it is always -1";
        return -1;
    }
    void setColumn(int column) override{
        Q_UNUSED (column)
        qmlWarning(this) << "column cannot be set on composite filters, the value will be ignored";
    };

    bool supportColumnFiltering() const override;

protected:
    QQmlCompositeFilterBase(QQmlFilterCompositorPrivate *priv, QObject *parent);
    void refreshCache() override;

private:
    Q_DECLARE_PRIVATE(QQmlCompositeFilterBase)
};

class QQmlCompositeFilterBasePrivate: public QQmlFilterCompositorPrivate
{
    Q_DECLARE_PUBLIC(QQmlCompositeFilterBase)
};

QT_END_NAMESPACE

#endif // QQMLCOMPOSITEFILTERBASE_P_H
