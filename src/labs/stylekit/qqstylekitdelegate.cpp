// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitdelegate_p.h"
#include "qqstylekitcontrolproperties_p.h"

#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qquickimplicitsizeitem_p_p.h>
#include <QtQuickControls2Impl/private/qquickcolorimage_p.h>

QT_BEGIN_NAMESPACE

QQStyleKitDelegate::QQStyleKitDelegate(QQuickItem *parent)
    : QQuickImplicitSizeItem(*new QQuickImplicitSizeItemPrivate, parent)
{
}

QQStyleKitDelegateProperties *QQStyleKitDelegate::delegateStyle() const
{
    return m_delegateProperties;
}

void QQStyleKitDelegate::setDelegateStyle(QQStyleKitDelegateProperties *delegateStyle)
{
    if (m_delegateProperties == delegateStyle)
        return;

    if (m_delegateProperties)
        disconnect(m_delegateProperties, nullptr, this, nullptr);

    m_delegateProperties = delegateStyle;

    if (!qmlEngine(this)) {
        qmlWarning(this) << "Unable to draw delegate: no QQmlEngine found";
        return;
    }

    maybeCreateColor();
    maybeCreateGradient();
    maybeCreateImage();
    updateImplicitSize();

    connect(m_delegateProperties, &QQStyleKitDelegateProperties::implicitWidthChanged, this, &QQStyleKitDelegate::updateImplicitSize);
    connect(m_delegateProperties, &QQStyleKitDelegateProperties::implicitHeightChanged, this, &QQStyleKitDelegate::updateImplicitSize);

    emit delegateStyleChanged();
}

void QQStyleKitDelegate::updateImplicitSize()
{
    if (!m_delegateProperties)
        return;

    /* The implicit size is determined by the following priority:
     * 1. Explicit implicit size set on QQStyleKitDelegateProperties
     * 2. Implicit size of the image (if present)
     * 3. Zero
     * The implicit size is read-only because it's calculated in C++ from internal
     * child items that are intentionally not exposed to QML. */
    const qreal impWidthInStyle = m_delegateProperties->implicitWidth();
    const qreal impHeightInStyle = m_delegateProperties->implicitHeight();
    setImplicitWidth(impWidthInStyle > 0 || !m_imageOverlay ? impWidthInStyle : m_imageOverlay->implicitWidth());
    setImplicitHeight(impHeightInStyle > 0 || !m_imageOverlay ? impHeightInStyle : m_imageOverlay->implicitHeight());
}

void QQStyleKitDelegate::maybeCreateColor()
{
    if (m_colorOverlay)
        return;
    if (!m_delegateProperties)
        return;
    if (m_delegateProperties->color().alpha() == 0
        && (m_delegateProperties->border()->color().alpha() == 0
            || m_delegateProperties->border()->width() == 0)) {
        // Lazy-create the color rectangle later, if/when needed
        connect(m_delegateProperties, &QQStyleKitDelegateProperties::colorChanged,
                this, &QQStyleKitDelegate::maybeCreateColor, Qt::UniqueConnection);
        connect(m_delegateProperties->border(), &QQStyleKitBorderProperties::colorChanged,
                this, &QQStyleKitDelegate::maybeCreateColor, Qt::UniqueConnection);
        connect(m_delegateProperties->border(), &QQStyleKitBorderProperties::widthChanged,
                this, &QQStyleKitDelegate::maybeCreateColor, Qt::UniqueConnection);
        return;
    }

    disconnect(m_delegateProperties, &QQStyleKitDelegateProperties::colorChanged,
            this, &QQStyleKitDelegate::maybeCreateColor);
    disconnect(m_delegateProperties->border(), &QQStyleKitBorderProperties::colorChanged,
            this, &QQStyleKitDelegate::maybeCreateColor);
    disconnect(m_delegateProperties->border(), &QQStyleKitBorderProperties::widthChanged,
            this, &QQStyleKitDelegate::maybeCreateColor);

    QQmlEngine *engine = qmlEngine(this);
    Q_ASSERT(engine);
    static QQmlComponent *component = nullptr;
    if (!component || component->engine() != engine) {
        delete component;
        component = new QQmlComponent(engine);
        const QString qmlCode = QString::fromUtf8(R"(
            import QtQuick
            Rectangle {
                z: -3
                width: parent.width
                height: parent.height
                color: delegateStyle.color
                opacity: delegateStyle.opacity
                topLeftRadius: delegateStyle.topLeftRadius
                topRightRadius: delegateStyle.topRightRadius
                bottomLeftRadius: delegateStyle.bottomLeftRadius
                bottomRightRadius: delegateStyle.bottomRightRadius
                border.width: delegateStyle.border.width
                border.color: delegateStyle.border.color
            }
        )");
        component->setData(qmlCode.toUtf8(), QUrl());
        Q_ASSERT_X(!component->isError(), __FUNCTION__, component->errorString().toUtf8().constData());
    }

    QQmlContext *ctx = QQmlEngine::contextForObject(this);
    m_colorOverlay = qobject_cast<QQuickItem*>(component->beginCreate(ctx));
    Q_ASSERT(m_colorOverlay);
    m_colorOverlay->setParent(this);
    m_colorOverlay->setParentItem(this);
    component->completeCreate();
}

void QQStyleKitDelegate::maybeCreateGradient()
{
    /* Unlike a Rectangle, a StyledItem draws both the color and the gradient at
     * the same time. This allows a style to define them independently. That way you can
     * define a common semi-transparent grayscale gradient once for a delegate in the style
     * (e.g for control.background.gradient), and then tint it with different colors for
     * different controls, states, or themes (e.g for button.hovered.background.color). */
    if (m_gradientOverlay)
        return;
    if (!m_delegateProperties)
        return;
    if (!m_delegateProperties->gradient()) {
        connect(m_delegateProperties, &QQStyleKitDelegateProperties::gradientChanged,
                this, &QQStyleKitDelegate::maybeCreateGradient, Qt::UniqueConnection);
        return;
    }

    disconnect(m_delegateProperties, &QQStyleKitDelegateProperties::gradientChanged,
               this, &QQStyleKitDelegate::maybeCreateGradient);

    QQmlEngine *engine = qmlEngine(this);
    Q_ASSERT(engine);
    static QQmlComponent *component = nullptr;
    if (!component || component->engine() != engine) {
        delete component;
        component = new QQmlComponent(engine);
        const QString qmlCode = QString::fromUtf8(R"(
            import QtQuick
            Rectangle {
                z: -2
                width: parent.width
                height: parent.height
                color: "transparent"
                gradient: delegateStyle.gradient
                topLeftRadius: delegateStyle.topLeftRadius
                topRightRadius: delegateStyle.topRightRadius
                bottomLeftRadius: delegateStyle.bottomLeftRadius
                bottomRightRadius: delegateStyle.bottomRightRadius
                border.width: delegateStyle.border.width
                border.color: delegateStyle.border.color
            }
        )");
        component->setData(qmlCode.toUtf8(), QUrl());
        Q_ASSERT_X(!component->isError(), __FUNCTION__, component->errorString().toUtf8().constData());
    }

    QQmlContext *ctx = QQmlEngine::contextForObject(this);
    m_gradientOverlay = qobject_cast<QQuickItem*>(component->beginCreate(ctx));
    Q_ASSERT(m_gradientOverlay);
    m_gradientOverlay->setParent(this);
    m_gradientOverlay->setParentItem(this);
    component->completeCreate();
}

void QQStyleKitDelegate::maybeCreateImage()
{
    if (m_imageOverlay)
        return;
    if (!m_delegateProperties)
        return;
    if (m_delegateProperties->image()->source().isEmpty()
        || m_delegateProperties->image()->color().alpha() == 0) {
        connect(m_delegateProperties->image(), &QQStyleKitImageProperties::sourceChanged,
                this, &QQStyleKitDelegate::maybeCreateImage, Qt::UniqueConnection);
        connect(m_delegateProperties->image(), &QQStyleKitImageProperties::colorChanged,
                this, &QQStyleKitDelegate::maybeCreateImage, Qt::UniqueConnection);
        return;
    }

    disconnect(m_delegateProperties->image(), &QQStyleKitImageProperties::sourceChanged,
               this, &QQStyleKitDelegate::maybeCreateImage);
    disconnect(m_delegateProperties->image(), &QQStyleKitImageProperties::colorChanged,
               this, &QQStyleKitDelegate::maybeCreateImage);

    QQmlEngine *engine = qmlEngine(this);
    Q_ASSERT(engine);
    static QQmlComponent *component = nullptr;
    if (!component || component->engine() != engine) {
        delete component;
        component = new QQmlComponent(engine);
        const QString qmlCode = QString::fromUtf8(R"(
            import QtQuick.Controls.impl
            ColorImage {
                z: -1
                width: parent.width
                height: parent.height
                color: delegateStyle.image.color
                source: delegateStyle.image.source
                fillMode: delegateStyle.image.fillMode
            }
        )");
        component->setData(qmlCode.toUtf8(), QUrl());
        Q_ASSERT_X(!component->isError(), __FUNCTION__, component->errorString().toUtf8().constData());
    }

    QQmlContext *ctx = QQmlEngine::contextForObject(this);
    m_imageOverlay = qobject_cast<QQuickItem*>(component->beginCreate(ctx));
    m_imageOverlay->setParent(this);
    m_imageOverlay->setParentItem(this);
    component->completeCreate();

    updateImplicitSize();
    connect(m_imageOverlay, &QQuickItem::implicitWidthChanged, this, &QQStyleKitDelegate::updateImplicitSize);
    connect(m_imageOverlay, &QQuickItem::implicitHeightChanged, this, &QQStyleKitDelegate::updateImplicitSize);
}

QT_END_NAMESPACE

#include "moc_qqstylekitdelegate_p.cpp"
