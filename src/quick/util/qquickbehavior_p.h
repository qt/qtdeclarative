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

#ifndef QQUICKBEHAVIOR_H
#define QQUICKBEHAVIOR_H

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

#include <private/qtquickglobal_p.h>

#include <private/qqmlpropertyvalueinterceptor_p.h>
#include <qqml.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractAnimation;
class QQuickBehaviorPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickBehavior : public QObject, public QQmlPropertyValueInterceptor
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickBehavior)

    Q_INTERFACES(QQmlPropertyValueInterceptor)
    Q_CLASSINFO("DefaultProperty", "animation")
    Q_PROPERTY(QQuickAbstractAnimation *animation READ animation WRITE setAnimation)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QVariant targetValue READ targetValue NOTIFY targetValueChanged REVISION 13)
    Q_PROPERTY(QQmlProperty targetProperty READ targetProperty NOTIFY targetPropertyChanged REVISION 15)
    Q_CLASSINFO("DeferredPropertyNames", "animation")
    QML_NAMED_ELEMENT(Behavior)

public:
    QQuickBehavior(QObject *parent=nullptr);
    ~QQuickBehavior();

    void setTarget(const QQmlProperty &) override;
    void write(const QVariant &value) override;

    QQuickAbstractAnimation *animation();
    void setAnimation(QQuickAbstractAnimation *);

    bool enabled() const;
    void setEnabled(bool enabled);

    QVariant targetValue() const;

    QQmlProperty targetProperty() const;

Q_SIGNALS:
    void enabledChanged();
    void targetValueChanged();
    void targetPropertyChanged();

private Q_SLOTS:
    void componentFinalized();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickBehavior)

#endif // QQUICKBEHAVIOR_H
