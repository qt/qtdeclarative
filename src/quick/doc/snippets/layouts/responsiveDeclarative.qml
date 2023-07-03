// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls

Window {
    visible: true
    width: 350
    height: 250
    //! [document]
    GridLayout {
        columns: width < 300 ? 1 : 2
        anchors.fill: parent

        Rectangle {
            id: rectangle1
            color: "tomato"
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        Rectangle {
            id: rectangle2
            color: "lightskyblue"
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
    //! [document]
}
