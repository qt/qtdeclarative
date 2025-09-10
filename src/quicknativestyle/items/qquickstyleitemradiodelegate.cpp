// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemradiodelegate.h"

#include <QtQuickTemplates2/private/qquickradiodelegate_p.h>

QT_BEGIN_NAMESPACE

void QQuickStyleItemRadioDelegate::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto radioDelegate = control<QQuickRadioDelegate>();
    connect(radioDelegate, &QQuickRadioDelegate::downChanged, this, &QQuickStyleItem::markImageDirty);
    connect(radioDelegate, &QQuickRadioDelegate::checkedChanged, this, &QQuickStyleItem::markImageDirty);
}

void QQuickStyleItemRadioDelegate::initStyleOption(QStyleOptionButton &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto radioDelegate = control<QQuickRadioDelegate>();

    styleOption.state |= radioDelegate->isDown() ? QStyle::State_Sunken : QStyle::State_Raised;
    styleOption.state |= radioDelegate->isChecked() ? QStyle::State_On : QStyle::State_Off;
}

QT_END_NAMESPACE

#include "moc_qquickstyleitemradiodelegate.cpp"
