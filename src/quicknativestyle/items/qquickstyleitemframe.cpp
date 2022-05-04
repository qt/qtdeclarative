/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
