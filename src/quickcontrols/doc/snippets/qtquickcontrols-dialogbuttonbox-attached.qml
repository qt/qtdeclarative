// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
