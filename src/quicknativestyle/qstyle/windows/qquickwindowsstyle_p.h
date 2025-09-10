// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKWINDOWSSTYLE_P_H
#define QQUICKWINDOWSSTYLE_P_H

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
#include <QBasicTimer>

QT_BEGIN_NAMESPACE

class QPalette;

namespace QQC2 {

class QStyleOptionButton;
class QWindowsStylePrivate;

class QWindowsStylePrivate;

class QWindowsStyle : public QCommonStyle
{
    Q_OBJECT
public:
    QWindowsStyle();
    ~QWindowsStyle() override;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p) const override;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p) const override;

    QRect subElementRect(SubElement r, const QStyleOption *opt) const override;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &contentsSize) const override;

    int pixelMetric(PixelMetric pm, const QStyleOption *option = nullptr) const override;

    int styleHint(StyleHint hint, const QStyleOption *opt = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override;

    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt) const override;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr) const override;
    void polish() override;

protected:
//    bool eventFilter(QObject *o, QEvent *e) override;
    QWindowsStyle(QWindowsStylePrivate &dd);
    void timerEvent(QTimerEvent * event) override;

private:
    Q_DISABLE_COPY_MOVE(QWindowsStyle)
    Q_DECLARE_PRIVATE(QWindowsStyle)
    QBasicTimer paletteTimer;

public Q_SLOTS:
    void refreshPalette();
};

} // namespace QQC2

QT_END_NAMESPACE

#endif // QQUICKWINDOWSSTYLE_P_H
