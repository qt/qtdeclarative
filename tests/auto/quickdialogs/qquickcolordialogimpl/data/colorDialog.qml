// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ApplicationWindow {
    width: 480
    height: 640

    color: "yellow"

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
    }
}
