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

#ifndef QQMLOBJECTORGADGET_P_H
#define QQMLOBJECTORGADGET_P_H

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

#include <private/qqmlmetaobject_p.h>

QT_BEGIN_NAMESPACE

class QQmlObjectOrGadget: public QQmlMetaObject
{
public:
    QQmlObjectOrGadget(QObject *obj)
        : QQmlMetaObject(obj),
        ptr(obj)
    {}
    QQmlObjectOrGadget(QQmlPropertyCache *propertyCache, void *gadget)
        : QQmlMetaObject(propertyCache)
        , ptr(gadget)
    {}

    void metacall(QMetaObject::Call type, int index, void **argv) const;

private:
    QBiPointer<QObject, void> ptr;

protected:
    QQmlObjectOrGadget(const QMetaObject* metaObject)
        : QQmlMetaObject(metaObject)
    {}
};

QT_END_NAMESPACE

#endif // QQMLOBJECTORGADGET_P_H
