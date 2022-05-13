// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ApplicationWindow {
    width: 640
    height: 480

    property alias dialog: dialog

    function doneAccepted() {
        dialog.done(FileDialog.Accepted)
    }

    function doneRejected() {
        dialog.done(FileDialog.Rejected)
    }

    FileDialog {
        id: dialog
        objectName: "FileDialog"
        options: FileDialog.DontUseNativeDialog
    }
}
