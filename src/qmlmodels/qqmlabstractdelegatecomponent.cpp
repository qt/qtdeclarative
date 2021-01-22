/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
