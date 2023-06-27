// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

//! [clickable window]
Rectangle {
    id: window

    color: "white"; width: 240; height: 150

    Column {
        anchors.centerIn: parent; spacing: 15

        MyClickableWidget {
            focus: true             //set this MyWidget to receive the focus
            color: "lightblue"
        }
        MyClickableWidget {
            color: "palegreen"
        }
    }

}
//! [clickable window]
