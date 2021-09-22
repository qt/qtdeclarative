/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.NativeStyle as NativeStyle

T.CheckBox {
    id: control

    readonly property bool nativeIndicator: indicator instanceof NativeStyle.StyleItem

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: nativeIndicator ? 0 : 6
    padding: nativeIndicator ? 0 : 6

    indicator: NativeStyle.CheckBox {
        control: control
        y: control.topPadding + (control.availableHeight - height) >> 1
        contentWidth: contentItem.implicitWidth
        contentHeight: contentItem.implicitHeight
        useNinePatchImage: false
    }

    contentItem: CheckLabel {
        text: control.text
        font: control.font
        color: control.palette.windowText

        // For some reason, the other styles set padding here (in the delegate), instead of in
        // the control above. And they also adjust the indicator position by setting x and y
        // explicitly (instead of using insets). So we follow the same pattern to ensure that
        // setting a custom contentItem delegate from the app will end up looking the same for
        // all styles. But this should probably be fixed for all styles (to make them work the
        // same way as e.g Buttons).
        leftPadding: {
            if (nativeIndicator)
                indicator.contentPadding.left
            else
                indicator && !mirrored ? indicator.width + spacing : 0
        }

        topPadding: nativeIndicator ? indicator.contentPadding.top : 0
        rightPadding: {
            if (nativeIndicator)
                indicator.contentPadding.right
            else
                indicator && mirrored ? indicator.width + spacing : 0
        }
    }
}
