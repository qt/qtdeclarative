// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

//! [file]
import QtQuick
import QtQuick.Controls

TextArea {
    id: control
    placeholderText: qsTr("Enter description")

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 40
        border.color: control.enabled ? "#21be2b" : "transparent"
    }
}
//! [file]
