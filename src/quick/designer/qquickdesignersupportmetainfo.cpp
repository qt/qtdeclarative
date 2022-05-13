// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

