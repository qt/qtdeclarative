/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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
**
**
**
******************************************************************************/

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
/*
    void polish(QApplication*) override;
    void unpolish(QApplication*) override;

    void polish(QWidget*) override;
    void unpolish(QWidget*) override;

    void polish(QPalette &) override;
*/
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

protected:
//    bool eventFilter(QObject *o, QEvent *e) override;
    QWindowsStyle(QWindowsStylePrivate &dd);

private:
    Q_DISABLE_COPY_MOVE(QWindowsStyle)
    Q_DECLARE_PRIVATE(QWindowsStyle)
};

} // namespace QQC2

QT_END_NAMESPACE

#endif // QQUICKWINDOWSSTYLE_P_H
