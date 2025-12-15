// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls.Basic

//! [ListView]
ListView {
    anchors.fill: parent
    model: ListModel {
        ListElement { hidden: false }
        ListElement { hidden: false }
        ListElement { hidden: false }
        // ...
    }
    delegate: ItemDelegate {
        text: qsTr("Item %1").arg(index)
        visible: !model.hidden
        height: visible ? implicitHeight : 0

        required property int index
        required property var model

        onClicked: model.hidden = true
    }
}
//! [ListView]
