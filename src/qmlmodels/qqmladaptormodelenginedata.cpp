// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include <private/qqmladaptormodelenginedata_p.h>
#include <private/qqmldmlistaccessordata_p.h>

QT_BEGIN_NAMESPACE

QQmlAdaptorModelEngineData::QQmlAdaptorModelEngineData(QV4::ExecutionEngine *v4)
    : v4(v4)
{
    QV4::Scope scope(v4);
    QV4::ScopedObject proto(scope, v4->newObject());
    proto->defineAccessorProperty(QStringLiteral("index"), get_index, nullptr);
    proto->defineAccessorProperty(
                QStringLiteral("modelData"),
                QQmlDMListAccessorData::get_modelData, QQmlDMListAccessorData::set_modelData);
    proto->defineAccessorProperty(
                QString(),
                QQmlDMListAccessorData::get_modelData, QQmlDMListAccessorData::set_modelData);
    listItemProto.set(v4, proto);
}

QT_END_NAMESPACE
