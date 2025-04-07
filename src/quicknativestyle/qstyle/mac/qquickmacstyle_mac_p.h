// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMACSTYLE_MAC_P_H
#define QMACSTYLE_MAC_P_H

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

#include "qquickcommonstyle.h"
#include "private/qglobal_p.h"

#define QQC2_NAMESPACE QQC2

QT_BEGIN_NAMESPACE

class QPalette;

namespace QQC2_NAMESPACE {

class QStyleOptionButton;
class QMacStylePrivate;

class QMacStyle : public QCommonStyle
{
    Q_OBJECT
public:
    QMacStyle();
    ~QMacStyle();

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p) const override;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p) const override;

    QRect subElementRect(SubElement r, const QStyleOption *opt) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc) const override;
    SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, const QPoint &pt) const override;

    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &contentsSize) const override;
    QFont font(ControlElement element, const QStyle::State state) const override;
    QMargins ninePatchMargins(ComplexControl cc, const QStyleOptionComplex *opt, const QSize &imageSize) const override;

    int pixelMetric(PixelMetric pm, const QStyleOption *opt = 0) const override;
    int styleHint(StyleHint sh, const QStyleOption *opt = 0, QStyleHintReturn *shret = 0) const override;

    QPixmap standardPixmap(StandardPixmap sp, const QStyleOption *opt) const override;
    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *opt) const override;

    void drawItemText(QPainter *p, const QRect &r, int flags, const QPalette &pal,
                              bool enabled, const QString &text,
                              QPalette::ColorRole textRole  = QPalette::NoRole) const override;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *opt = 0) const override;

    void handleThemeChange() override;

private:
    Q_DISABLE_COPY_MOVE(QMacStyle)
    Q_DECLARE_PRIVATE(QMacStyle)
};

} // QQC2_NAMESPACE

QT_END_NAMESPACE

#endif // QMACSTYLE_MAC_P_H
