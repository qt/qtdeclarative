// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

//! [window]

//Window code that imports MyWidget
Rectangle {
    id: window
    color: "white"; width: 240; height: 150

    Column {
        anchors.centerIn: parent; spacing: 15

        MyWidget {
            focus: true             //set this MyWidget to receive the focus
            color: "lightblue"
        }
        MyWidget {
            color: "palegreen"
        }
    }
}
//! [window]
