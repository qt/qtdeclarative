// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    width: 480
    height: 640

    property FileDialog dialog

    Component {
        id: dialogComponent
        FileDialog {
            objectName: "FileDialog"
        }
    }

    // Explicitly make the window, rather than an item, the dialog's parent.
    Component.onCompleted: dialog = dialogComponent.createObject(root)
}
