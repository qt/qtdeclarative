// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Controls.Basic

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
