// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Analog of ApplicationFlow.qml: instantiates the derived Home and attaches an
// external handler to the nested button through the alias, exactly like
// ApplicationFlow's "getStartedbutton.onClicked". This anchors the button's
// stash context to the outer CU's group-property view.
import QtQuick

Item {
    id: outer
    property alias home: home
    property alias theButton: home.theButton
    property int activations: 0
    RevertHome {
        id: home
        theButton.onActivated: outer.activations++
    }
}
