// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.RadioButton {
    id: control
    objectName: "radiobutton-simple"

    implicitWidth: contentItem.implicitWidth + indicator.implicitWidth
    implicitHeight: Math.max(contentItem.implicitHeight, indicator.implicitHeight)

    indicator: Text {
        objectName: "radiobutton-indicator-simple"
        text: control.checked ? "O" : ""
    }

    contentItem: Text {
        objectName: "radiobutton-contentItem-simple"
        text: control.text
    }
}
