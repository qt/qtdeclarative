/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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

QT_BEGIN_NAMESPACE

class QQmlListReferencePrivate
{
public:
    QQmlListReferencePrivate();

    static QQmlListReference init(const QQmlListProperty<QObject> &, QMetaType, QQmlEngine *);

    QPointer<QObject> object;
    QQmlListProperty<QObject> property;
    QMetaType propertyType;

    void addref();
    void release();
    int refCount;

    static inline QQmlListReferencePrivate *get(QQmlListReference *ref) {
        return ref->d;
    }

    void setEngine(QQmlEngine *engine)
    {
        m_elementTypeOrEngine = engine;
    }

    const QMetaObject *elementType()
    {
        if (m_elementTypeOrEngine.isT2()) {
            const int listType = QQmlMetaType::listType(propertyType).id();
            const QQmlEngine *engine = m_elementTypeOrEngine.asT2();
            const QQmlEnginePrivate *p = engine ? QQmlEnginePrivate::get(engine) : nullptr;
            m_elementTypeOrEngine = p ? p->rawMetaObjectForType(listType).metaObject()
                                      : QQmlMetaType::qmlType(listType).baseMetaObject();
        }

        return m_elementTypeOrEngine.asT1();
    }

private:
    QBiPointer<const QMetaObject, QQmlEngine> m_elementTypeOrEngine;
};


QT_END_NAMESPACE

#endif // QQMLLIST_P_H
