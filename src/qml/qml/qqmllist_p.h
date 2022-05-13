// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLLIST_P_H
#define QQMLLIST_P_H

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

#include "qqmllist.h"
#include "qqmlmetaobject_p.h"
#include "qqmlmetatype_p.h"
#include "qqmlengine_p.h"
#include <QtQml/private/qbipointer_p.h>

QT_BEGIN_NAMESPACE

class QQmlListReferencePrivate
{
public:
    QQmlListReferencePrivate();

    static QQmlListReference init(const QQmlListProperty<QObject> &, QMetaType);

    QPointer<QObject> object;
    QQmlListProperty<QObject> property;
    QMetaType propertyType;

    void addref();
    void release();
    int refCount;

    static inline QQmlListReferencePrivate *get(QQmlListReference *ref) {
        return ref->d;
    }

    const QMetaObject *elementType()
    {
        if (!m_elementType) {
            m_elementType = QQmlMetaType::rawMetaObjectForType(
                        QQmlMetaType::listValueType(propertyType)).metaObject();
        }

        return m_elementType;
    }

private:
    const QMetaObject *m_elementType = nullptr;
};


QT_END_NAMESPACE

#endif // QQMLLIST_P_H
