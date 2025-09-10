// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemcheckdelegate.h"

#include <QtQuickTemplates2/private/qquickcheckdelegate_p.h>

QT_BEGIN_NAMESPACE

void QQuickStyleItemCheckDelegate::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto checkDelegate = control<QQuickCheckDelegate>();
    connect(checkDelegate, &QQuickCheckDelegate::downChanged, this, &QQuickStyleItem::markImageDirty);
    connect(checkDelegate, &QQuickCheckDelegate::checkStateChanged, this, &QQuickStyleItem::markImageDirty);
}

void QQuickStyleItemCheckDelegate::initStyleOption(QStyleOptionButton &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto checkDelegate = control<QQuickCheckDelegate>();

    styleOption.state |= checkDelegate->isDown() ? QStyle::State_Sunken : QStyle::State_Raised;
    if (checkDelegate->isTristate() && checkDelegate->checkState() == Qt::PartiallyChecked)
        styleOption.state |= QStyle::State_NoChange;
    else
        styleOption.state |= checkDelegate->isChecked() ? QStyle::State_On : QStyle::State_Off;
}

QT_END_NAMESPACE

#include "moc_qquickstyleitemcheckdelegate.cpp"
