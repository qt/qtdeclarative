// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include <private/qqmldmobjectdata_p.h>

QT_BEGIN_NAMESPACE

QQmlDMObjectData::QQmlDMObjectData(const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
        VDMObjectDelegateDataType *dataType,
        int index, int row, int column,
        QObject *object)
    : QQmlDelegateModelItem(metaType, dataType, index, row, column)
    , object(object)
{
    new QQmlDMObjectDataMetaObject(this, dataType);
}

QT_END_NAMESPACE
