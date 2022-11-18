// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

//! [1]
ListView {
    width: 160
    height: 240

    model: Qt.fontFamilies()

    delegate: ItemDelegate {
        text: modelData
        width: parent.width
        onClicked: console.log("clicked:", modelData)

        required property string modelData
    }

    ScrollIndicator.vertical: ScrollIndicator { }
}
//! [1]
