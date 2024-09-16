// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.ComboBox {
    id: control
    objectName: "combobox-simple"

    implicitWidth: Math.max(contentItem.implicitWidth + indicator.implicitWidth, background.implicitWidth)
    implicitHeight: Math.max(contentItem.implicitHeight, indicator.implicitHeight, background.implicitHeight)

    indicator: Text {
        objectName: "combobox-indicator-simple"
        text: control.comboed ? "V" : ""
    }

    contentItem: Text {
        objectName: "combobox-contentItem-simple"
        text: control.currentText
    }

    background: Rectangle {
        objectName: "combobox-background-simple"
        implicitWidth: 20
        implicitHeight: 20
        color: control.down ? "red" : "green"
    }

    popup: T.Popup {
        objectName: "combobox-popup-simple"
    }
}
