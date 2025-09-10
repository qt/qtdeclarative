// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKWINDOWSSTYLE_P_P_H
#define QQUICKWINDOWSSTYLE_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of XX.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include "qquickwindowsstyle_p.h"
#include "qquickcommonstyle_p.h"
#include "qquickstylehelper_p.h"

QT_BEGIN_NAMESPACE

namespace QQC2 {

class QStyleOptionButton;
class QWindowsStylePrivate;

#include <qlist.h>

class QTime;

class QWindowsStylePrivate : public QCommonStylePrivate
{
    Q_DECLARE_PUBLIC(QWindowsStyle)
public:
    enum { InvalidMetric = -23576 };

    QWindowsStylePrivate();
    static int pixelMetricFromSystemDp(QStyle::PixelMetric pm, const QStyleOption *option = nullptr);
    static int fixedPixelMetric(QStyle::PixelMetric pm);
    static qreal devicePixelRatio(const QStyleOption *option = nullptr)
    {
        return devicePixelRatio(option ? option->window : nullptr);
    }

    static qreal devicePixelRatio(const QWindow *win)
        { return win ? win->devicePixelRatio() : QWindowsStylePrivate::appDevicePixelRatio(); }
    static qreal nativeMetricScaleFactor(const QStyleOption *option = nullptr);
    static qreal nativeMetricScaleFactor(const QWindow *win);
    static bool isDarkMode();

#if 0
    bool hasSeenAlt(const QWidget *widget) const;
    bool altDown() const { return alt_down; }
    bool alt_down = false;
#endif
    QList<const QWidget *> seenAlt;
    int menuBarTimer = 0;

    QColor inactiveCaptionText;
    QColor activeCaptionColor;
    QColor activeGradientCaptionColor;
    QColor inactiveCaptionColor;
    QColor inactiveGradientCaptionColor;

    enum {
        windowsItemFrame        =  2, // menu item frame width
        windowsSepHeight        =  9, // separator item height
        windowsItemHMargin      =  3, // menu item hor text margin
        windowsItemVMargin      =  2, // menu item ver text margin
        windowsArrowHMargin     =  6, // arrow horizontal margin
        windowsRightBorder      = 15, // right border on windows
        windowsCheckMarkWidth   = 12  // checkmarks width on windows
    };

private:
    static qreal appDevicePixelRatio();
};

} // namespace QQC2

QT_END_NAMESPACE

#endif //QQUICKWINDOWSSTYLE_P_P_H

