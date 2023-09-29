// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef NODEINSTANCEMETAOBJECT_H
#define NODEINSTANCEMETAOBJECT_H

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

#include "qquickdesignersupport_p.h"

#include <QQmlContext>
#include <QScopedPointer>
#include <private/qqmlopenmetaobject_p.h>
#include <private/qqmlvmemetaobject_p.h>

#include <QtCore/qpointer.h>

#include <memory>

QT_BEGIN_NAMESPACE

struct MetaPropertyData;

class QQmlDesignerMetaObject : public QQmlVMEMetaObject
{
public:
    ~QQmlDesignerMetaObject();

    static void registerNotifyPropertyChangeCallBack(void (*callback)(QObject*, const QQuickDesignerSupport::PropertyName &propertyName));

protected:
    static QQmlDesignerMetaObject* getNodeInstanceMetaObject(QObject *object, QQmlEngine *engine);

    void createNewDynamicProperty(const QString &name);
    int openMetaCall(QObject *o, QMetaObject::Call _c, int _id, void **_a);
    int metaCall(QObject *o, QMetaObject::Call _c, int _id, void **_a) override;
    void notifyPropertyChange(int id);
    void setValue(int id, const QVariant &value);
    QVariant propertyWriteValue(int, const QVariant &);

    QObject *myObject() const { return QQmlVMEMetaObject::object; }

    QDynamicMetaObjectData *dynamicMetaObjectParent() const;

    const QMetaObject *metaObjectParent() const;

    int propertyOffset() const;

    int count() const;
    QByteArray name(int) const;

    void copyTypeMetaObject();

private:
    QQmlDesignerMetaObject(QObject *object, QQmlEngine *engine);
    void init(QObject *);
    QQmlOpenMetaObjectType *type() const { return m_openMetaObject->type(); }

    QPointer<QQmlContext> m_context;
    std::unique_ptr<QQmlOpenMetaObject> m_openMetaObject;
    QScopedPointer<MetaPropertyData> m_data;

    // This is non-const. You cannot use threads when using the designer metaobject.
    // Otherwise it's the same as QQmlVMEMetaObject's "cache" member.
    QQmlPropertyCache::Ptr m_cache;

    friend class QQuickDesignerSupportProperties;
};

QT_END_NAMESPACE

#endif // NODEINSTANCEMETAOBJECT_H
