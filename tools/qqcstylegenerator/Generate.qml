// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.qmlmodels
import QtCore

ColumnLayout {
    implicitWidth: parent.width
    implicitHeight: childrenRect.height

    property string figmaFileName

    Connections {
        target: bridge

        function onFinished() {
            progressBar.value = 0
            progressBar.indeterminate = false
            statusLabel.text = window.stopping ? "Stopped!" : "Finished!"
        }

        function onProgressToChanged(to) {
            if (window.stopping)
                return
            progressBar.to = to
            progressBar.value = 0
            progressBar.indeterminate = to === 0
        }

        function onProgressLabelChanged(label) {
            if (window.stopping)
                return
            statusLabel.text = label
        }

        function onProgress() {
            progressBar.value++
        }

        function onFigmaFileNameChanged(name) {
            figmaFileName = name
        }
    }

    Label {
        text: figmaFileName
    }

    Label {
        id: statusLabel
    }

    ProgressBar {
        id: progressBar
        Layout.fillWidth: true
    }

    Button {
        id: generateButton
        Layout.alignment: Qt.AlignRight
        text: window.generating ? "Stop" : "Generate"
        enabled: !window.stopping
                 && bridge.figmaUrlOrId !== ""
                 && bridge.figmaToken !== ""
                 && bridge.targetDirectory !== ""
        onClicked: {
            if (window.generating) {
                window.stopping = true
                statusLabel.text = "Stopping..."
                bridge.stop()
            } else {
                figmaFileName = ""
                bridge.generate()
            }
        }
        Component.onCompleted: forceActiveFocus()
    }
}

