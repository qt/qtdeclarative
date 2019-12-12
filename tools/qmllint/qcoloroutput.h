/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOLOROUTPUT_H
#define QCOLOROUTPUT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/qglobal.h>
#include <QtCore/qscopedpointer.h>

class ColorOutputPrivate;

class ColorOutput
{
    enum
    {
        ForegroundShift = 10,
        BackgroundShift = 20,
        SpecialShift    = 20,
        ForegroundMask  = 0x1f << ForegroundShift,
        BackgroundMask  = 0x7 << BackgroundShift
    };

public:
    enum ColorCodeComponent
    {
        BlackForeground         = 1 << ForegroundShift,
        BlueForeground          = 2 << ForegroundShift,
        GreenForeground         = 3 << ForegroundShift,
        CyanForeground          = 4 << ForegroundShift,
        RedForeground           = 5 << ForegroundShift,
        PurpleForeground        = 6 << ForegroundShift,
        BrownForeground         = 7 << ForegroundShift,
        LightGrayForeground     = 8 << ForegroundShift,
        DarkGrayForeground      = 9 << ForegroundShift,
        LightBlueForeground     = 10 << ForegroundShift,
        LightGreenForeground    = 11 << ForegroundShift,
        LightCyanForeground     = 12 << ForegroundShift,
        LightRedForeground      = 13 << ForegroundShift,
        LightPurpleForeground   = 14 << ForegroundShift,
        YellowForeground        = 15 << ForegroundShift,
        WhiteForeground         = 16 << ForegroundShift,

        BlackBackground         = 1 << BackgroundShift,
        BlueBackground          = 2 << BackgroundShift,
        GreenBackground         = 3 << BackgroundShift,
        CyanBackground          = 4 << BackgroundShift,
        RedBackground           = 5 << BackgroundShift,
        PurpleBackground        = 6 << BackgroundShift,
        BrownBackground         = 7 << BackgroundShift,
        DefaultColor            = 1 << SpecialShift
    };

    using ColorCode = QFlags<ColorCodeComponent>;
    using ColorMapping = QHash<int, ColorCode>;

    ColorOutput(bool silent);
    ~ColorOutput();

    void insertMapping(int colorID, ColorCode colorCode);

    void writeUncolored(const QString &message);
    void write(const QString &message, int color = -1);
    QString colorify(const QString &message, int color = -1) const;

private:
    QScopedPointer<ColorOutputPrivate> d;
    Q_DISABLE_COPY_MOVE(ColorOutput)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ColorOutput::ColorCode)

#endif // QCOLOROUTPUT_H
