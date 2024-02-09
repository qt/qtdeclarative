// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtCore
import QtQuick
import QtQuick.VectorImage
import QtQuick.Controls
import QtQuick.Dialogs

Window {
    width: 1920
    height: 1080
    title: qsTr("Vector Image")
    visible: true

    Column {
        spacing: 20

        Button {
            width: 100
            height: 50
            text: qsTr("Select Svg file")
            onClicked: fileDialog.open()
        }

        VectorImage {
            id: vectorImage
            source: "qrc:/res/spheres.svg"
        }
    }

    FileDialog {
        id: fileDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        onAccepted: vectorImage.source = selectedFile
    }
}
