// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include <QtQmlModels/private/qqmlabstractdelegatecomponent_p.h>
#include <QtQmlModels/private/qqmladaptormodel_p.h>

QT_BEGIN_NAMESPACE

QQmlAbstractDelegateComponent::QQmlAbstractDelegateComponent(QObject *parent)
    : QQmlComponent(parent)
{
}

QQmlAbstractDelegateComponent::~QQmlAbstractDelegateComponent()
{
}

QVariant QQmlAbstractDelegateComponent::value(QQmlAdaptorModel *adaptorModel, int row, int column, const QString &role) const
{
    if (!adaptorModel)
        return QVariant();
    return adaptorModel->value(adaptorModel->indexAt(row, column), role);
}

QT_END_NAMESPACE

#include "moc_qqmlabstractdelegatecomponent_p.cpp"
