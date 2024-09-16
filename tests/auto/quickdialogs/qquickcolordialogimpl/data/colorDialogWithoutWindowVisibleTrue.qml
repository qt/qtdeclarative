// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

Rectangle {
    width: 480
    height: 640
    color: dialog.selectedColor

    property alias dialog: dialog

    function doneAccepted() {
        dialog.done(ColorDialog.Accepted)
    }

    function doneRejected() {
        dialog.done(ColorDialog.Rejected)
    }

    ColorDialog {
        id: dialog
        objectName: "ColorDialog"
	visible: true
    }
}
