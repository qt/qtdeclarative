// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.qmlmodels
import QtCore

ApplicationWindow {
    id: window
    width: 800
    height: 600
    visible: true
    title: "Qt Figma Style Generator"

    property bool generating: false
    property bool stopping: false
    property bool closeRequested: false

    onClosing: (closeEvent) => {
        if (generating) {
            stopping = true
            bridge.stop()
            closeRequested = true
            closeEvent.accepted = false
        }
    }

    Connections {
        target: bridge
        function onStarted() {
            generating = true
        }

        function onFinished() {
            generating = false
            stopping = false
            if (closeRequested)
                window.close()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        Image {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 50
            source: Qt.resolvedUrl("background.png")
            Image {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 10
                source: Qt.resolvedUrl("figma_and_qt.png")
                fillMode: Image.PreserveAspectFit
            }
        }

        ScrollView {
            id: scrollView
            Layout.preferredWidth: parent.width
            Layout.fillHeight: true
            contentHeight: column.childrenRect.height
            ColumnLayout {
                id: column
                width: scrollView.width - scrollView.effectiveScrollBarWidth - 10
                spacing: 20
                FigmaOptions { }
                ControlsOptions { }
                ExportOptions { }
                Log { }
                Usage { }
            }
        }

        Item { height: 10 }
        Generate { }
    }
}

