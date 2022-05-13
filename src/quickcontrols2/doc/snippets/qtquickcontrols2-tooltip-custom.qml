// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

//! [file]
import QtQuick
import QtQuick.Controls

Item {
ToolTip {
    id: control
    text: qsTr("A descriptive tool tip of what the button does")

    contentItem: Text {
        text: control.text
        font: control.font
        color: "#21be2b"
    }

    background: Rectangle {
        border.color: "#21be2b"
    }
}
}
//! [file]
