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
    modal: false
    standardButtons: Dialog.Ok
}
//! [1]
}
