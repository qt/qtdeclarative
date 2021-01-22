/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickdesignercustomparserobject_p.h"
#include "qquickdesignersupportmetainfo_p.h"
#include "qqmldesignermetaobject_p.h"

#include <private/qqmlmetatype_p.h>

QT_BEGIN_NAMESPACE

bool QQuickDesignerSupportMetaInfo::isSubclassOf(QObject *object, const QByteArray &superTypeName)
{
    if (object == nullptr)
        return false;

    const QMetaObject *metaObject = object->metaObject();

    while (metaObject) {
         QQmlType qmlType =  QQmlMetaType::qmlType(metaObject);
         if (qmlType.qmlTypeName() == QLatin1String(superTypeName)) // ignore version numbers
             return true;

         if (metaObject->className() == superTypeName)
             return true;

         metaObject = metaObject->superClass();
    }

    return false;
}

void QQuickDesignerSupportMetaInfo::registerNotifyPropertyChangeCallBack(void (*callback)(QObject *, const QQuickDesignerSupport::PropertyName &))
{
    QQmlDesignerMetaObject::registerNotifyPropertyChangeCallBack(callback);
}

void QQuickDesignerSupportMetaInfo::registerMockupObject(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    qmlRegisterCustomType<QQuickDesignerCustomParserObject>(uri, versionMajor, versionMinor, qmlName, new QQuickDesignerCustomParser);
}

QT_END_NAMESPACE

