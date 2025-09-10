// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlobjectorgadget_p.h"

QT_BEGIN_NAMESPACE

void QQmlObjectOrGadget::metacall(QMetaObject::Call type, int index, void **argv) const
{
    if (ptr.isNull()) {
        _m->d.static_metacall(nullptr, type, index, argv);
    }
    else if (ptr.isT1()) {
        QMetaObject::metacall(ptr.asT1(), type, index, argv);
    }
    else {
        const QMetaObject *metaObject = _m;
        QQmlMetaObject::resolveGadgetMethodOrPropertyIndex(type, &metaObject, &index);
        metaObject->d.static_metacall(reinterpret_cast<QObject*>(ptr.asT2()), type, index, argv);
    }
}

QT_END_NAMESPACE
