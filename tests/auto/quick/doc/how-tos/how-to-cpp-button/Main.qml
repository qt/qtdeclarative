// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//! [file]
import QtQuick.Controls

import MyModule

ApplicationWindow {
    width: 400
    height: 400
    title: qsTr("C++ Button example")

    Button {
        text: qsTr("Click me")
        onClicked: Backend.doStuff()
    }
}
//! [file]
