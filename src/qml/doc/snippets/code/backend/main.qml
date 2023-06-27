// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [main_qml]
import QtQuick
import QtQuick.Controls
//![import]
import io.qt.examples.backend 1.0
//![import]

ApplicationWindow {
    id: root
    width: 300
    height: 480
    visible: true

//![backend]
    BackEnd {
        id: backend
    }
//![backend]

//![username_input]
    TextField {
        text: backend.userName
        placeholderText: qsTr("User name")
        anchors.centerIn: parent

        onEditingFinished: backend.userName = text
    }
//![username_input]
}
//! [main_qml]
