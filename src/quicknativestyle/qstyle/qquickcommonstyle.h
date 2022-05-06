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

#ifndef QCOMMONSTYLE_H
#define QCOMMONSTYLE_H

#include "qquickstyle.h"

QT_BEGIN_NAMESPACE

namespace QQC2 {

class QCommonStylePrivate;

class QCommonStyle: public QStyle
{
    Q_OBJECT

public:
    QCommonStyle();
    ~QCommonStyle() override;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p) const override;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p) const override;

    QRect subElementRect(SubElement r, const QStyleOption *opt) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc) const override;

    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &contentsSize) const override;
    QFont font(ControlElement element, const QStyle::State state) const override;
    QMargins ninePatchMargins(ControlElement ce, const QStyleOption *opt, const QSize &imageSize) const override;
    QMargins ninePatchMargins(ComplexControl cc, const QStyleOptionComplex *opt, const QSize &imageSize) const override;

    int pixelMetric(PixelMetric m, const QStyleOption *opt = nullptr) const override;
    int styleHint(StyleHint sh, const QStyleOption *opt = nullptr, QStyleHintReturn *shret = nullptr) const override;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *opt = nullptr) const override;
    QPixmap standardPixmap(StandardPixmap sp, const QStyleOption *opt = nullptr) const override;
    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *opt) const override;

    SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, const QPoint &pt) const override;

protected:
    QCommonStyle(QCommonStylePrivate &dd);

private:
    Q_DECLARE_PRIVATE(QCommonStyle)
    Q_DISABLE_COPY(QCommonStyle)
};

} // namespace QQC2

QT_END_NAMESPACE

#endif // QCOMMONSTYLE_H
