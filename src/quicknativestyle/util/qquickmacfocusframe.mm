/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickmacfocusframe.h"

#include <AppKit/AppKit.h>

#include <QtCore/qmetaobject.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/private/qcoregraphics_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquicktextinput_p.h>
#include <QtQuick/private/qquicktextedit_p.h>
#include <QtQuick/private/qquickflickable_p.h>

#include <QtQuickTemplates2/private/qquickframe_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTemplates2/private/qquickscrollview_p.h>
#include <QtQuickTemplates2/private/qquickslider_p.h>
#include <QtQuickTemplates2/private/qquickcombobox_p.h>
#include <QtQuickTemplates2/private/qquickcheckbox_p.h>
#include <QtQuickTemplates2/private/qquickradiobutton_p.h>
#include <QtQuickTemplates2/private/qquickspinbox_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>

#include "items/qquickstyleitem.h"
#include "qquicknativestyle.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcFocusFrame, "qt.quick.controls.focusframe")

QQuickFocusFrameDescription QQuickFocusFrameDescription::Invalid = { nullptr, QQuickStyleMargins(), 0 };
QScopedPointer<QQuickItem> QQuickMacFocusFrame::m_focusFrame;

QQuickMacFocusFrame::QQuickMacFocusFrame()
{
    connect(qGuiApp, &QGuiApplication::focusObjectChanged, [=]{
        if (auto item = qobject_cast<QQuickItem *>(qGuiApp->focusObject()))
            moveToItem(item);
    });
}

void QQuickMacFocusFrame::moveToItem(QQuickItem *item)
{
    if (!m_focusFrame) {
        const auto context = QQmlEngine::contextForObject(item);
        if (!context)
            return;
        createFocusFrame(context);
    }

    const QQuickFocusFrameDescription &config = getDescriptionForItem(item);
    QMetaObject::invokeMethod(m_focusFrame.data(), "moveToItem",
                              Q_ARG(QVariant, QVariant::fromValue(config.target)),
                              Q_ARG(QVariant, QVariant::fromValue(config.margins)),
                              Q_ARG(QVariant, QVariant::fromValue(config.radius)));
}

void QQuickMacFocusFrame::createFocusFrame(QQmlContext *context)
{
    QQmlComponent component(
            context->engine(),
            QUrl(QStringLiteral(
                    "qrc:/qt-project.org/imports/QtQuick/NativeStyle/util/FocusFrame.qml")));
    m_focusFrame.reset(qobject_cast<QQuickItem *>(component.create()));

    auto indicatorColor = qt_mac_toQColor(NSColor.keyboardFocusIndicatorColor.CGColor);
    indicatorColor.setAlpha(255);
    m_focusFrame->setProperty("systemFrameColor", indicatorColor);
}

QQuickFocusFrameDescription QQuickMacFocusFrame::getDescriptionForItem(QQuickItem *focusItem) const
{
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
    // has a "__isDefaultDelegate" property set, we show a default focus frame instead.
    if (item->property("__isDefaultDelegate").toBool() == true) {
        qCDebug(lcFocusFrame) << "'__isDefaultDelegate' property found, showing a default focus frame";
        const QStyleOption opt;
        const qreal radius = QQuickNativeStyle::style()->pixelMetric(QStyle::PM_TextFieldFocusFrameRadius, &opt);
        return { target, QQuickStyleMargins(), radius };
    }

    // The application has set a custom delegate on the control. In that
    // case, it's the delegates responsibility to draw a focus frame.
    qCDebug(lcFocusFrame) << "custom delegates in use, skip showing focus frame";
    return QQuickFocusFrameDescription::Invalid;
}

QT_END_NAMESPACE
