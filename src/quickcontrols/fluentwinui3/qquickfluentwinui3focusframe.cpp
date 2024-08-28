// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfluentwinui3focusframe_p.h"

#include <QtCore/qmetaobject.h>

#include <QtGui/qguiapplication.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>

#include <QtQuick/qquickitem.h>


QT_BEGIN_NAMESPACE

QScopedPointer<QQuickItem> QQuickFluentWinUI3FocusFrame::m_focusFrame;

QQuickFluentWinUI3FocusFrame::QQuickFluentWinUI3FocusFrame()
{
    connect(qGuiApp, &QGuiApplication::focusObjectChanged, this, [this]{
        if (auto item = qobject_cast<QQuickItem *>(qGuiApp->focusObject()))
            moveToItem(item);
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

void QQuickFluentWinUI3FocusFrame::moveToItem(QQuickItem *item)
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
    }

    const auto target = getFocusTarget(item);
    QMetaObject::invokeMethod(m_focusFrame.data(), "moveToItem",
                              Q_ARG(QVariant, QVariant::fromValue(target)));
}

QQuickItem *QQuickFluentWinUI3FocusFrame::getFocusTarget(QQuickItem *focusItem) const
{
    const auto parentItem = focusItem->parentItem();
    if (!parentItem)
        return nullptr;

    // The item that gets active focus can be a child of the control (e.g
    // editable ComboBox). In that case, resolve the actual control first.
    const auto proxy = focusItem->property("__focusFrameControl").value<QQuickItem *>();
    const auto control = proxy ? proxy : focusItem;
    auto target = control->property("__focusFrameTarget").value<QQuickItem *>();

    return target;
}

QT_END_NAMESPACE
