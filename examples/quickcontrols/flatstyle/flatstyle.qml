// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import Qt.labs.platform
import Theme

ApplicationWindow {
    id: window
    visible: true
    minimumWidth: 360
    height: 480

    title: qsTr("Flat Style")

    MainForm {
        id: form

        anchors.fill: parent
        button.onClicked: colorDialog.open()

        sizeSwitch.onCheckedChanged: Theme.baseSize = (sizeSwitch.checked ? Theme.largeSize : Theme.smallSize)
        checkBoxBold.onCheckedChanged: Theme.font.bold = checkBoxBold.checked
        checkBoxUnderline.onCheckedChanged: Theme.font.underline = checkBoxUnderline.checked
        slider.onPositionChanged: Theme.font.pixelSize = slider.valueAt(slider.position)
    }

    ColorDialog {
        id: colorDialog
        onCurrentColorChanged: Theme.mainColor = currentColor
    }
}
