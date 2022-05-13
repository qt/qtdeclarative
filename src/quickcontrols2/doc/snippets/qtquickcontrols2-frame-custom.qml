// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

//! [file]
import QtQuick
import QtQuick.Controls

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
