// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Controls.Basic

Frame {
    background: Rectangle {
        color: "transparent"
        border.color: "#21be2b"
        radius: 2
    }

    Label {
        text: qsTr("Content goes here!")
    }
}
//! [file]
