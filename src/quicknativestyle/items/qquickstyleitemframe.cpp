// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemframe.h"

QT_BEGIN_NAMESPACE

StyleItemGeometry QQuickStyleItemFrame::calculateGeometry()
{
    QStyleOptionFrame styleOption;
    initStyleOption(styleOption);
    StyleItemGeometry geometry;

    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_Frame, &styleOption, QSize(0, 0));
    geometry.implicitSize = contentSize();
    styleOption.rect = QRect(QPoint(0, 0), geometry.implicitSize);
    geometry.contentRect = style()->subElementRect(QStyle::SE_FrameContents, &styleOption);
    geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CE_ShapedFrame, &styleOption, geometry.minimumSize);

    return geometry;
}

void QQuickStyleItemFrame::paintEvent(QPainter *painter) const
{
    QStyleOptionFrame styleOption;
    initStyleOption(styleOption);
    style()->drawControl(QStyle::CE_ShapedFrame, &styleOption, painter);
}

void QQuickStyleItemFrame::initStyleOption(QStyleOptionFrame &styleOption) const
{
    initStyleOptionBase(styleOption);
    styleOption.lineWidth = 1;
    styleOption.frameShape = QStyleOptionFrame::StyledPanel;
    styleOption.features = QStyleOptionFrame::Flat;
}

QT_END_NAMESPACE

#include "moc_qquickstyleitemframe.cpp"
