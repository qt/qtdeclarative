// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblequickpopupitem_p.h"
#include "qquickpopup_p.h"
#include "qquickpopupitem_p_p.h"

QT_BEGIN_NAMESPACE

QAccessibleQuickPopupItem::QAccessibleQuickPopupItem(QQuickPopupItem *popupItem)
    : QAccessibleQuickItem(popupItem)
{
}

QAccessible::State QAccessibleQuickPopupItem::state() const
{
    QAccessible::State state = QAccessibleQuickItem::state();

    QQuickPopup *popup = qobject_cast<QQuickPopup *>(object()->parent());
    if (popup)
        state.modal = popup->isModal();

    return state;
}

QT_END_NAMESPACE
