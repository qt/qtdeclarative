// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfluentwinui3focusframe_p.h"

#include <private/qquickitem_p.h>

#include <QtCore/qmetaobject.h>

#include <QtGui/qguiapplication.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>


QT_BEGIN_NAMESPACE

QScopedPointer<QQuickItem> QQuickFluentWinUI3FocusFrame::m_focusFrame;

QQuickFluentWinUI3FocusFrame::QQuickFluentWinUI3FocusFrame()
{
    connect(qGuiApp, &QGuiApplication::focusObjectChanged, this, [this](QObject *focusObject){
        if (QQuickControl *control = qobject_cast<QQuickControl *>(focusObject);
            control && (control->focusReason() == Qt::FocusReason::TabFocusReason
                    || control->focusReason() == Qt::FocusReason::BacktabFocusReason
                    || control->focusReason() == Qt::FocusReason::OtherFocusReason)) {
                        moveToItem(control);
        } else {
            moveToItem(nullptr);
        }
    });
}

QQuickItem *QQuickFluentWinUI3FocusFrame::createFocusFrame(QQmlContext *context)
{
    QQmlComponent component(
            context->engine(),
            QUrl(QStringLiteral(
                "qrc:/qt-project.org/imports/QtQuick/Controls/FluentWinUI3/FocusFrame.qml")));
    auto frame = qobject_cast<QQuickItem *>(component.create());
    if (!frame)
        return nullptr;
    return frame;
}

void QQuickFluentWinUI3FocusFrame::moveToItem(QQuickControl *item)
{
    if (!m_focusFrame) {
        const auto context = QQmlEngine::contextForObject(item);
        // In certain cases like QQuickWebEngineView, the item
        // gets focus even though it has no QQmlEngine associated with its context.
        // We need the engine for creating the focus frame component.
        if (!context || !context->engine())
            return;
        m_focusFrame.reset(createFocusFrame(context));
        QQuickItemPrivate::get(m_focusFrame.get())->setTransparentForPositioner(true);
        if (!m_focusFrame) {
            qWarning() << "Failed to create FocusFrame";
            return;
        }
    }

    const auto target = getFocusTarget(item);
    QMetaObject::invokeMethod(m_focusFrame.data(), "moveToItem",
                              Q_ARG(QVariant, QVariant::fromValue(target)));
}

QQuickControl *QQuickFluentWinUI3FocusFrame::getFocusTarget(QQuickControl *focusItem) const
{
    if (!focusItem)
        return nullptr;

    const auto parentItem = focusItem->parentItem();
    if (!parentItem)
        return nullptr;

    // The control that gets active focus can be a child of the control (e.g
    // editable ComboBox). In that case, resolve the actual control first.
    const auto proxy = focusItem->property("__focusFrameControl").value<QQuickControl *>();
    const auto control = proxy ? proxy : focusItem;
    auto target = control->property("__focusFrameTarget").value<QQuickControl *>();

    return target;
}

QT_END_NAMESPACE
