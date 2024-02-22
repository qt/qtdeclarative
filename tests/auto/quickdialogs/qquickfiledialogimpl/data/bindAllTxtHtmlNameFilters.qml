// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ApplicationWindow {
    width: 640
    height: 480

    property alias dialog: dialog

    FileDialog {
        id: dialog
        objectName: "FileDialog"
        nameFilters: ["All files (*)", "Text files (*.txt)", "HTML files (*.html)"]
    }
}
