// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

Item {
    width: dialog.implicitWidth
    height: dialog.implicitHeight
//! [1]
Dialog {
    id: dialog
    title: "Title"
    standardButtons: Dialog.Ok | Dialog.Cancel

    onAccepted: console.log("Ok clicked")
    onRejected: console.log("Cancel clicked")
}
//! [1]
}
