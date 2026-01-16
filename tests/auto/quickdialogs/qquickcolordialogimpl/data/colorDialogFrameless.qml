// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ApplicationWindow {
    width: 480
    height: 640

    property alias dialog: dialog

    ColorDialog {
        id: dialog
        objectName: "ColorDialog"
        flags: Qt.Window | Qt.FramelessWindowHint
    }
}
