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

#ifndef QQMLSTATICMETAOBJECT_P_H
#define QQMLSTATICMETAOBJECT_P_H

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

#include <private/qqmlobjectorgadget_p.h>

QT_BEGIN_NAMESPACE

class QQmlStaticMetaObject : public QQmlObjectOrGadget {
public:
    QQmlStaticMetaObject(const QMetaObject* metaObject)
        : QQmlObjectOrGadget(metaObject)
    {}
    int *constructorParameterTypes(int index, ArgTypeStorage *dummy, QByteArray *unknownTypeError) const;
};

QT_END_NAMESPACE

#endif // QQMLSTATICMETAOBJECT_P_H
