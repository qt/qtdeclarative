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

#ifndef DESIGNERCUSTOMOBJECTDATA_H
#define DESIGNERCUSTOMOBJECTDATA_H

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

#include <QHash>
#include <QObject>
#include <QVariant>

#include <private/qqmlbinding_p.h>

QT_BEGIN_NAMESPACE
class QQmlContext;

class QQuickDesignerCustomObjectData
{
public:
    static void registerData(QObject *object);
    static QQuickDesignerCustomObjectData *get(QObject *object);
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
    void handleDestroyed();

private:
    QQuickDesignerCustomObjectData(QObject *object);
    void populateResetHashes();
    QObject *object() const;
    QVariant getResetValue(const QQuickDesignerSupport::PropertyName &propertyName) const;
    void doResetProperty(QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName);
    bool hasValidResetBinding(const QQuickDesignerSupport::PropertyName &propertyName) const;
    QQmlAbstractBinding *getResetBinding(const QQuickDesignerSupport::PropertyName &propertyName) const;
    bool hasBindingForProperty(QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName, bool *hasChanged) const;
    void setPropertyBinding(QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName, const QString &expression);
    void keepBindingFromGettingDeleted(QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName);

    QObject *m_object;
    QHash<QQuickDesignerSupport::PropertyName, QVariant> m_resetValueHash;
    QHash<QQuickDesignerSupport::PropertyName, QQmlAbstractBinding::Ptr> m_resetBindingHash;
    mutable QHash<QQuickDesignerSupport::PropertyName, bool> m_hasBindingHash;
};

QT_END_NAMESPACE

#endif // DESIGNERCUSTOMOBJECTDATA_H
