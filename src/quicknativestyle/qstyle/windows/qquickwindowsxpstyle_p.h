// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKWINDOWSXPSTYLE_P_H
#define QQUICKWINDOWSXPSTYLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquickwindowsstyle_p.h"

QT_BEGIN_NAMESPACE

namespace QQC2 {

class QWindowsXPStylePrivate;
class QWindowsXPStyle : public QWindowsStyle
{
    Q_OBJECT
public:
    QWindowsXPStyle();
    QWindowsXPStyle(QWindowsXPStylePrivate &dd);
    ~QWindowsXPStyle() override;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *option, QPainter *p) const override;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *p) const override;
    QRect subElementRect(SubElement r, const QStyleOption *option) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *option, SubControl sc) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *option, QPainter *p) const override;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *option, const QSize &contentsSize) const override;
    int pixelMetric(PixelMetric pm, const QStyleOption *option = nullptr) const override;
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override;

    QPalette standardPalette() const override;
    QPixmap standardPixmap(StandardPixmap standardIcon, const QStyleOption *option) const override;
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr) const override;

    QMargins ninePatchMargins(QStyle::ComplexControl cc, const QStyleOptionComplex *opt, const QSize &imageSize) const override;

private:
    Q_DISABLE_COPY_MOVE(QWindowsXPStyle)
    Q_DECLARE_PRIVATE(QWindowsXPStyle)
    friend class QStyleFactory;
};

} // namespace QQC2

QT_END_NAMESPACE

#endif // QQUICKWINDOWSXPSTYLE_P_H
