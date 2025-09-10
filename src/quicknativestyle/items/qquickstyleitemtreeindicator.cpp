// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemtreeindicator.h"

QT_BEGIN_NAMESPACE

void QQuickStyleItemTreeIndicator::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto delegate = control<QQuickTreeViewDelegate>();
    connect(delegate, &QQuickTreeViewDelegate::expandedChanged, this, &QQuickStyleItem::markImageDirty);
}

StyleItemGeometry QQuickStyleItemTreeIndicator::calculateGeometry()
{
    QStyleOptionViewItem styleOption;
    initStyleOption(styleOption);
    StyleItemGeometry geometry;

    geometry.minimumSize = QSize(20, 20);
    geometry.implicitSize = geometry.minimumSize;
    styleOption.rect = QRect(QPoint(0, 0), geometry.implicitSize);

    return geometry;
}

void QQuickStyleItemTreeIndicator::paintEvent(QPainter *painter) const
{
    QStyleOptionViewItem styleOption;
    initStyleOption(styleOption);
    style()->drawPrimitive(QStyle::PE_IndicatorBranch, &styleOption, painter);
}

void QQuickStyleItemTreeIndicator::initStyleOption(QStyleOptionViewItem &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto delegate = control<QQuickTreeViewDelegate>();

    styleOption.state |= QStyle::State_Children;

    if (delegate->expanded())
        styleOption.state |= QStyle::State_Open;
}

QT_END_NAMESPACE

#include "moc_qquickstyleitemtreeindicator.cpp"
