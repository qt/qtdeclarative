// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Window
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 640
    height: 600
    AboutDialog {
        id: aboutDialog
        anchors.centerIn: parent
    }
    WasmMenu {
        id: wasmMenu
        Accessible.focusable: true
        focusPolicy: Qt.StrongFocus
        focus: true
        property string timeCaption: "Initiated at :"
        anchors {
            left: parent.left
            leftMargin: 20
            top: parent.top
        }
        function getCurrentDate() {
            var currentDate = new Date()
            var year = currentDate.getFullYear()
            var month = currentDate.getMonth() + 1
            var day = currentDate.getDate()
            return day + "/" + month + "/" + year
        }
        function getCurrentTime() {
            var currentDate = new Date()
            var hours = currentDate.getHours()
            var minutes = currentDate.getMinutes()
            var seconds = currentDate.getSeconds()
            return hours + ":" + minutes + ":" + seconds
        }
        function removeTextAfterIndex(rmText, currdateTime) {
            var index = currdateTime.indexOf(rmText)
            if (index !== -1) {
                currdateTime = currdateTime.substring(0, index)
            }
            return currdateTime
        }
        onShowTime: {
            timeCaption = removeTextAfterIndex(", time:", timeCaption)
            timeCaption += ", time: " + getCurrentTime()
            meetingTabs.setTime.text = timeCaption
        }
        onShowDate: {
            timeCaption = removeTextAfterIndex(" date:", timeCaption)
            timeCaption += " date: " + getCurrentDate()
            meetingTabs.setTime.text = timeCaption
        }
        onShowAboutDialog: {
            aboutDialog.open()
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
        enabled: meetingTabs.currentIndex === MeetingTabs.Types.Summary ? true : false
    }

    Rectangle {
        width: parent.width - 30
        height: parent.height - wasmToolbar.height - wasmMenu.height - 30
        border.color: "black"
        border.width: 1
        id:outerRect
        anchors {
            left: parent.left
            leftMargin: 20
            top: wasmToolbar.bottom
            topMargin: 10
            bottomMargin: 10
        }

        MeetingTabs {
            id: meetingTabs
            parent:outerRect
            anchors {
                centerIn: parent
            }
            height: parent.height - 20
            width: parent.width - 20
        }
    }
}
