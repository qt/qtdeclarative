// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfocusframe.h"

#include <private/qquickitem_p.h>

#include <QtCore/qmetaobject.h>

#include <QtGui/qguiapplication.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>

#include <QtQuick/qquickitem.h>

#include "items/qquickstyleitem.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcFocusFrame, "qt.quick.controls.focusframe")

QQuickFocusFrameDescription QQuickFocusFrameDescription::Invalid = { nullptr, QQuickStyleMargins(), 0 };
QScopedPointer<QQuickItem> QQuickFocusFrame::m_focusFrame;

QQuickFocusFrame::QQuickFocusFrame()
{
    connect(qGuiApp, &QGuiApplication::focusObjectChanged, this, [this](QObject *focusObject){
        if (auto item = qobject_cast<QQuickItem *>(focusObject))
            moveToItem(item);
        else
            moveToItem(nullptr);
    });
}

void QQuickFocusFrame::moveToItem(QQuickItem *item)
{
    if (!m_focusFrame) {
        const auto context = QQmlEngine::contextForObject(item);
        // In certain cases like QQuickWebEngineView, the item
        // gets focus even though it has no QQmlEngine associated with its context.
        // We need the engine for creating the focus frame component.
        if (!context || !context->engine())
            return;
        m_focusFrame.reset(createFocusFrame(context));
        if (!m_focusFrame) {
            qWarning() << "Failed to create FocusFrame";
            return;
        }
        QQuickItemPrivate::get(m_focusFrame.get())->setTransparentForPositioner(true);
    }

    const QQuickFocusFrameDescription &config = getDescriptionForItem(item);
    QMetaObject::invokeMethod(m_focusFrame.data(), "moveToItem",
                              Q_ARG(QVariant, QVariant::fromValue(config.target)),
                              Q_ARG(QVariant, QVariant::fromValue(config.margins)),
                              Q_ARG(QVariant, QVariant::fromValue(config.radius)));
}

QQuickFocusFrameDescription QQuickFocusFrame::getDescriptionForItem(QQuickItem *focusItem) const
{
    if (!focusItem)
        return QQuickFocusFrameDescription::Invalid;

    qCDebug(lcFocusFrame) << "new focusobject:" << focusItem;
    const auto parentItem = focusItem->parentItem();
    if (!parentItem)
        return QQuickFocusFrameDescription::Invalid;

    // The item that gets active focus can be a child of the control (e.g
    // editable ComboBox). In that case, resolve the actual control first.
    const auto proxy = focusItem->property("__focusFrameControl").value<QQuickItem *>();
    const auto control = proxy ? proxy : focusItem;
    auto target = control->property("__focusFrameTarget").value<QQuickItem *>();
    qCDebug(lcFocusFrame) << "target:" << target;
    qCDebug(lcFocusFrame) << "control:" << control;

    if (!target) {
        // __focusFrameTarget points to the item in the control that should
        // get the focus frame. This is usually the control itself, but can
        // sometimes be a child (CheckBox). We anyway require
        // this property to be set if we are to show the focus frame around
        // the control in the first place. So for controls that don't want
        // a frame (ProgressBar), we simply skip setting it.
        // Also, we should never show a focus frame around custom controls.
        // None of the built-in styles do that, so to be consistent, we
        // shouldn't either. Besides, drawing a focus frame around an unknown
        // item without any way to turn it off can easily be unwanted. A better
        // way for custom controls to get a native focus frame is for us to offer
        // a FocusFrame control (QTBUG-86818).
        return QQuickFocusFrameDescription::Invalid;
    }

    // If the control gives us a QQuickStyleItem, we use that to configure the focus frame.
    // By default we assume that the background delegate is a QQuickStyleItem, but the
    // control can override this by setting __focusFrameStyleItem.
    const auto styleItemProperty = control->property("__focusFrameStyleItem");
    auto item = styleItemProperty.value<QQuickItem *>();
    if (!item) {
        const auto styleItemProperty = control->property("background");
        item = styleItemProperty.value<QQuickItem *>();
    }
    qCDebug(lcFocusFrame) << "styleItem:" << item;
    if (!item)
        return QQuickFocusFrameDescription::Invalid;
    if (QQuickStyleItem *styleItem = qobject_cast<QQuickStyleItem *>(item))
        return { target, QQuickStyleMargins(styleItem->layoutMargins()), styleItem->focusFrameRadius() };

    // Some controls don't have a QQuickStyleItem. But if the __focusFrameStyleItem
    // has a "__focusFrameRadius" property set, we show a default focus frame using the specified radius instead.
    const QVariant focusFrameRadiusVariant = item->property("__focusFrameRadius");
    if (focusFrameRadiusVariant.isValid()) {
        qCDebug(lcFocusFrame) << "'focusFrameRadius' property found, showing a default focus frame";
        const QStyleOption opt;
        const qreal radius = qMax(0.0, focusFrameRadiusVariant.toReal());
        return { target, QQuickStyleMargins(), radius };
    }

    // The application has set a custom delegate on the control. In that
    // case, it's the delegates responsibility to draw a focus frame.
    qCDebug(lcFocusFrame) << "custom delegates in use, skip showing focus frame";
    return QQuickFocusFrameDescription::Invalid;
}

QT_END_NAMESPACE
