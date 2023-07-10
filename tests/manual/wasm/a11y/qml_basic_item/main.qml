// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Window
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 640
    height: 480

    WasmMenu {
        id: wasmMenu
        anchors {
            left: parent.left
            leftMargin: 20
            top: parent.top
        }
    }

    WasmToolBar {
        id: wasmToolbar
        anchors {
            left: parent.left
            leftMargin: 20
            top: wasmMenu.bottom
            topMargin: 3
        }
    }

   Rectangle {

       width: 600
       height: 480
       border.color: "black"
       border.width: 1
       anchors {
           left: parent.left
           leftMargin: 20
           top: wasmToolbar.bottom
           topMargin: 10
       }

       MeetingTabs {

           anchors {
               left: parent.left
               leftMargin: 5
               top: parent.top
               topMargin: 5
           }
        width: 550
        height: 480
    }
   }
}
