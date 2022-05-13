// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

//! [1]
DialogButtonBox {
    Button {
        text: qsTr("Save")
        DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
    }
    Button {
        text: qsTr("Close")
        DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
    }
}
//! [1]
