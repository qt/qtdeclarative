// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuick/private/qquickitem_p.h>

#include "qqstylekitdelegatecontainer_p.h"
#include "../qqstylekitcontrolproperties_p.h"

QT_BEGIN_NAMESPACE

QQmlComponent* QQStyleKitDelegateContainer::s_defaultDelegateComponent = nullptr;
QQmlComponent* QQStyleKitDelegateContainer::s_defaultShadowComponent = nullptr;

QQStyleKitDelegateContainer::QQStyleKitDelegateContainer(QQuickItem *parent)
    : QQuickItem(parent)
{
}

QQStyleKitDelegateContainer::~QQStyleKitDelegateContainer()
{
    // Disconnect before QQuickItem::~QQuickItem() reparents children, which emits
    // signals. At that point our vtable would already be replaced with QQuickItem's,
    // causing assertObjectType to fail.
    if (m_delegateInstance)
        disconnect(m_delegateInstance, nullptr, this, nullptr);
}

QQStyleKitDelegateProperties *QQStyleKitDelegateContainer::delegateProperties() const
{
    return m_delegateProperties;
}

void QQStyleKitDelegateContainer::setDelegateProperties(QQStyleKitDelegateProperties *delegateProperties)
{
    if (m_delegateProperties == delegateProperties)
        return;

    if (m_delegateProperties) {
        /* We don't expect delegateProperties to change after componentCompleted(), since it's bound
         * to a controls StyleKitReader, which is not supposed to change. So, considering that there
         * might be hundreds of delegate instances in an application, we try to save some connections
         * this way. But note, this is only an optimization, not a technical limitation. */
        qmlWarning(this) << "Changing delegateProperties on StyleKitContainer is not supported.";
        return;
    }

    m_delegateProperties = delegateProperties;
    emit delegatePropertiesChanged();
}

QObject *QQStyleKitDelegateContainer::parentControl() const
{
    return m_control;
}

void QQStyleKitDelegateContainer::setParentControl(QObject *control)
{
    if (m_control == control)
        return;
    m_control = control;
    emit parentControlChanged();
}

QQuickItem *QQStyleKitDelegateContainer::delegateInstance() const
{
    return m_delegateInstance;
}

void QQStyleKitDelegateContainer::updateImplicitSize()
{
    setImplicitWidth(m_delegateInstance ? m_delegateInstance->implicitWidth() : 0);
    setImplicitHeight(m_delegateInstance ? m_delegateInstance->implicitHeight() : 0);
}

void QQStyleKitDelegateContainer::maybeCreateDelegate()
{
    if (m_delegateInstance)
        return;
    if (!m_delegateProperties)
        return;

    if (!m_delegateProperties->visible()) {
        connect(m_delegateProperties, &QQStyleKitDelegateProperties::visibleChanged,
                this, &QQStyleKitDelegateContainer::maybeCreateDelegate, Qt::UniqueConnection);
        return;
    }

    disconnect(m_delegateProperties, &QQStyleKitDelegateProperties::visibleChanged,
               this, &QQStyleKitDelegateContainer::maybeCreateDelegate);

    if (QQmlComponent *delegateComponent = m_delegateProperties->delegate()) {
        m_delegateComponent = delegateComponent;
    } else {
        if (!s_defaultDelegateComponent || s_defaultDelegateComponent->engine() != qmlEngine(this)) {
            delete s_defaultDelegateComponent;
            QQmlEngine *engine = qmlEngine(this);
            s_defaultDelegateComponent = new QQmlComponent(engine);
            const QString qmlCode = QString::fromUtf8(R"(
                import QtQuick
                import Qt.labs.StyleKit
                StyleKitDelegate {
                    width: parent.width
                    height: parent.height
                }
            )");
            s_defaultDelegateComponent->setData(qmlCode.toUtf8(), QUrl());
            Q_ASSERT_X(!s_defaultDelegateComponent->isError(), __FUNCTION__,
                       s_defaultDelegateComponent->errorString().toUtf8().constData());
        }
        m_delegateComponent = s_defaultDelegateComponent;
    }

    QQmlContext *ctx = QQmlEngine::contextForObject(this);
    m_delegateInstance = qobject_cast<QQuickItem*>(m_delegateComponent->beginCreate(ctx));
    Q_ASSERT(m_delegateInstance);
    m_delegateInstance->setParent(this);
    m_delegateInstance->setParentItem(this);
    m_delegateInstance->setProperty("control", QVariant::fromValue(m_control.get()));
    m_delegateInstance->setProperty("delegateProperties", QVariant::fromValue(m_delegateProperties.get()));
    m_delegateComponent->completeCreate();

    updateImplicitSize();
    connect(m_delegateInstance, &QQuickItem::implicitWidthChanged, this, &QQStyleKitDelegateContainer::updateImplicitSize);
    connect(m_delegateInstance, &QQuickItem::implicitHeightChanged, this, &QQStyleKitDelegateContainer::updateImplicitSize);
}

void QQStyleKitDelegateContainer::maybeCreateShadow()
{
    if (m_shadowInstance)
        return;
    if (!m_delegateProperties)
        return;

    if (!m_delegateProperties->visible()) {
        connect(m_delegateProperties, &QQStyleKitDelegateProperties::visibleChanged,
                this, &QQStyleKitDelegateContainer::maybeCreateShadow, Qt::UniqueConnection);
        return;
    }
    if (!m_delegateProperties->shadow()->visible()) {
        connect(m_delegateProperties->shadow(), &QQStyleKitShadowProperties::visibleChanged,
                this, &QQStyleKitDelegateContainer::maybeCreateShadow, Qt::UniqueConnection);
        return;
    }
    if (m_delegateProperties->shadow()->color().alpha() == 0) {
        connect(m_delegateProperties->shadow(), &QQStyleKitShadowProperties::colorChanged,
                this, &QQStyleKitDelegateContainer::maybeCreateShadow, Qt::UniqueConnection);
        return;
    }
    if (m_delegateProperties->shadow()->opacity() == 0) {
        connect(m_delegateProperties->shadow(), &QQStyleKitShadowProperties::opacityChanged,
                this, &QQStyleKitDelegateContainer::maybeCreateShadow, Qt::UniqueConnection);
        return;
    }

    disconnect(m_delegateProperties, &QQStyleKitDelegateProperties::visibleChanged,
            this, &QQStyleKitDelegateContainer::maybeCreateShadow);
    disconnect(m_delegateProperties->shadow(), &QQStyleKitShadowProperties::visibleChanged,
            this, &QQStyleKitDelegateContainer::maybeCreateShadow);
    disconnect(m_delegateProperties->shadow(), &QQStyleKitShadowProperties::colorChanged,
            this, &QQStyleKitDelegateContainer::maybeCreateShadow);
    disconnect(m_delegateProperties->shadow(), &QQStyleKitShadowProperties::opacityChanged,
            this, &QQStyleKitDelegateContainer::maybeCreateShadow);

    if (QQmlComponent *shadowComponent = m_delegateProperties->shadow()->delegate()) {
        m_shadowComponent = shadowComponent;
    } else {
        if (!s_defaultShadowComponent || s_defaultShadowComponent->engine() != qmlEngine(this)) {
            delete s_defaultShadowComponent;
            QQmlEngine *engine = qmlEngine(this);
            s_defaultShadowComponent = new QQmlComponent(engine);
            const QString qmlCode = QString::fromUtf8(R"(
                import Qt.labs.StyleKit.impl
                Shadow {}
            )");
            s_defaultShadowComponent->setData(qmlCode.toUtf8(), QUrl());
            Q_ASSERT_X(!s_defaultShadowComponent->isError(), __FUNCTION__,
                       s_defaultShadowComponent->errorString().toUtf8().constData());
        }
        m_shadowComponent = s_defaultShadowComponent;
    }

    QQmlContext *ctx = QQmlEngine::contextForObject(this);
    m_shadowInstance = qobject_cast<QQuickItem*>(m_shadowComponent->beginCreate(ctx));
    Q_ASSERT(m_shadowInstance);
    m_shadowInstance->setParent(this);
    m_shadowInstance->setParentItem(this);
    m_shadowInstance->setZ(-1);
    m_shadowInstance->setProperty("control", QVariant::fromValue(m_control.get()));
    m_shadowInstance->setProperty("delegateProperties", QVariant::fromValue(m_delegateProperties.get()));
    m_shadowComponent->completeCreate();
}

void QQStyleKitDelegateContainer::componentComplete()
{
    QQuickItem::componentComplete();
    Q_ASSERT(m_delegateProperties);
    Q_ASSERT(m_control);

    maybeCreateDelegate();
    connect(m_delegateProperties, &QQStyleKitDelegateProperties::delegateChanged, this, [this]{
        if (!m_delegateInstance) {
            maybeCreateDelegate();
        } else {
            const QQmlComponent *newDelegateComp = m_delegateProperties->delegate();
            if (m_delegateComponent == newDelegateComp)
                return;
            if (!newDelegateComp && m_delegateComponent == s_defaultDelegateComponent) {
                /* If newDelegateComp is nullptr, it means that we should use the default
                 * delegate instead, which we already do. */
                return;
            }

            delete m_delegateInstance;
            maybeCreateDelegate();
            emit delegateInstanceChanged();
        }
    });

    maybeCreateShadow();
    connect(m_delegateProperties->shadow(), &QQStyleKitShadowProperties::delegateChanged, this, [this]{
        if (!m_shadowInstance) {
            maybeCreateShadow();
        } else {
            const QQmlComponent *newShadowComp = m_delegateProperties->shadow()->delegate();
            if (m_shadowComponent == newShadowComp)
                return;
            if (!newShadowComp && m_shadowComponent == s_defaultShadowComponent) {
                /* If newShadowComp is nullptr, it means that we should use the default
                 * delegate instead, which we already do. */
                return;
            }

            delete m_shadowInstance;
            maybeCreateShadow();
        }
    });
}

QT_END_NAMESPACE

#include "moc_qqstylekitdelegatecontainer_p.cpp"
