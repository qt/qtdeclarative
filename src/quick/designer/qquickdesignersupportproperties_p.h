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

#ifndef DESIGNERSUPPORTPROPERTIES_H
#define DESIGNERSUPPORTPROPERTIES_H

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

#include <QObject>
#include <QString>
#include <QVariant>
#include <QList>
#include <QByteArray>
#include <QQmlContext>
#include <QQmlListReference>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickDesignerSupportProperties
{
public:
    static void createNewDynamicProperty(QObject *object, QQmlEngine *engine, const QString &name);
    static void registerNodeInstanceMetaObject(QObject *object, QQmlEngine *engine);
    static void registerCustomData(QObject *object);
    static QVariant getResetValue(QObject *object, const QQuickDesignerSupport::PropertyName &propertyName);
    static void doResetProperty(QObject *object, QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName);
    static bool hasValidResetBinding(QObject *object, const QQuickDesignerSupport::PropertyName &propertyName);

    static bool hasBindingForProperty(QObject *object,
                                      QQmlContext *context,
                                      const QQuickDesignerSupport::PropertyName &propertyName,
                                      bool *hasChanged);
    static void setPropertyBinding(QObject *object,
                                   QQmlContext *context,
                                   const QQuickDesignerSupport::PropertyName &propertyName,
                                   const QString &expression);
    static void keepBindingFromGettingDeleted(QObject *object,
                                              QQmlContext *context,
                                              const QQuickDesignerSupport::PropertyName &propertyName);

    static bool isPropertyQObject(const QMetaProperty &metaProperty);
    static QObject *readQObjectProperty(const QMetaProperty &metaProperty, QObject *object);

    static void getPropertyCache(QObject *object, QQmlEngine *engine);
    static bool isPropertyBlackListed(const QQuickDesignerSupport::PropertyName &propertyName);
    static QQuickDesignerSupport::PropertyNameList propertyNameListForWritableProperties(QObject *object,
                                                                  const QQuickDesignerSupport::PropertyName &baseName = QQuickDesignerSupport::PropertyName(),
                                                                  QObjectList *inspectedObjects = nullptr);
    static QQuickDesignerSupport::PropertyNameList allPropertyNames(QObject *object,
                                             const QQuickDesignerSupport::PropertyName &baseName = QQuickDesignerSupport::PropertyName(),
                                             QObjectList *inspectedObjects = nullptr);
    static bool hasFullImplementedListInterface(const QQmlListReference &list);
};

QT_END_NAMESPACE

#endif // DESIGNERSUPPORTPROPERTIES_H
