// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

Window {
    width: 480
    height: 640

    property alias dialog: dialog

    function getSubWindow1 () {
        return subwindow1
    }

    function getSubWindow2 () {
        return subwindow2
    }

    function goToSubWindow1() {
        dialog.close()
        dialog.parentWindow = subwindow1
        dialog.open()
    }

    function goToSubWindow2() {
        dialog.close()
        dialog.parentWindow = subwindow2
        dialog.open()
    }

    function resetParentWindow() {
        dialog.close()
        dialog.parentWindow = undefined
        dialog.open()
    }

    Window {
        id: subwindow1
        width: 480
        height: 640
        visible: true
    }

    Window {
        id: subwindow2
        width: 480
        height: 640
        visible: true
    }

    ColorDialog {
        id: dialog
        objectName: "ColorDialog"
    }
}
