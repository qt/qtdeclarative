// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    width: 640
    height: 480

    required property url tempFile1Url
    required property int fileMode

    property alias dialog: dialog

    FileDialog {
        id: dialog
        objectName: "FileDialog"
        fileMode: root.fileMode
        selectedFile: root.tempFile1Url
    }
}
