// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemdelaybutton.h"

#include <QtQuickTemplates2/private/qquickdelaybutton_p.h>

QT_BEGIN_NAMESPACE

void QQuickStyleItemDelayButton::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto button = control<QQuickDelayButton>();
    connect(button, &QQuickButton::downChanged, this, &QQuickStyleItem::markImageDirty);
    connect(button, &QQuickButton::checkedChanged, this, &QQuickStyleItem::markImageDirty);
}

void QQuickStyleItemDelayButton::initStyleOption(QStyleOptionButton &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto button = control<QQuickDelayButton>();

    styleOption.state |= button->isDown() ? QStyle::State_Sunken : QStyle::State_Raised;
}

QT_END_NAMESPACE

#include "moc_qquickstyleitemdelaybutton.cpp"
