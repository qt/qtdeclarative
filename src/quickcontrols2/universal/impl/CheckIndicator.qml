/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.Universal

Rectangle {
    id: indicator
    implicitWidth: 20
    implicitHeight: 20

    color: !control.enabled ? "transparent" :
            control.down && !partiallyChecked ? control.Universal.baseMediumColor :
            control.checkState === Qt.Checked ? control.Universal.accent : "transparent"
    border.color: !control.enabled ? control.Universal.baseLowColor :
                   control.down ? control.Universal.baseMediumColor :
                   control.checked ? control.Universal.accent : control.Universal.baseMediumHighColor
    border.width: 2 // CheckBoxBorderThemeThickness

    property Item control
    readonly property bool partiallyChecked: control.checkState === Qt.PartiallyChecked

    ColorImage {
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        visible: indicator.control.checkState === Qt.Checked
        color: !indicator.control.enabled ? indicator.control.Universal.baseLowColor : indicator.control.Universal.chromeWhiteColor
        source: "qrc:/qt-project.org/imports/QtQuick/Controls/Universal/images/checkmark.png"
    }

    Rectangle {
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: indicator.partiallyChecked ? parent.width / 2 : parent.width
        height: indicator.partiallyChecked ? parent.height / 2 : parent.height

        visible: !indicator.control.pressed && indicator.control.hovered || indicator.partiallyChecked
        color: !indicator.partiallyChecked ? "transparent" :
               !indicator.control.enabled ? indicator.control.Universal.baseLowColor :
                indicator.control.down ? indicator.control.Universal.baseMediumColor :
                indicator.control.hovered ? indicator.control.Universal.baseHighColor : indicator.control.Universal.baseMediumHighColor
        border.width: indicator.partiallyChecked ? 0 : 2 // CheckBoxBorderThemeThickness
        border.color: indicator.control.Universal.baseMediumLowColor
    }
}
