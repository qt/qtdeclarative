// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

//! [1]
Column {
    ButtonGroup {
        id: childGroup
        exclusive: false
        checkState: parentBox.checkState
    }

    CheckBox {
        id: parentBox
        text: qsTr("Parent")
        checkState: childGroup.checkState
    }

    CheckBox {
        checked: true
        text: qsTr("Child 1")
        leftPadding: indicator.width
        ButtonGroup.group: childGroup
    }

    CheckBox {
        text: qsTr("Child 2")
        leftPadding: indicator.width
        ButtonGroup.group: childGroup
    }
}
//! [1]
