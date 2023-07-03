// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [document]
import QtQuick

Rectangle {
    width: 300; height: 55
    color: Qt.application.active ? "white" : "lightgray"
    Text {
        text: "Application " + (Qt.application.active ? "active" : "inactive")
        opacity: Qt.application.active ? 1.0 : 0.5
        anchors.centerIn: parent
    }
}
//! [document]
