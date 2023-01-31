// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_QML_BUILD_REMOVED_API

#include "qtqmlglobal.h"

QT_USE_NAMESPACE

#if QT_QML_REMOVED_SINCE(6, 5)

#include <QtQml/qjsengine.h>

QJSValue QJSEngine::create(int typeId, const void *ptr)
{
    QMetaType type(typeId);
    return create(type, ptr);
}

bool QJSEngine::convertV2(const QJSValue &value, int type, void *ptr)
{
    return convertV2(value, QMetaType(type), ptr);
}

#endif

