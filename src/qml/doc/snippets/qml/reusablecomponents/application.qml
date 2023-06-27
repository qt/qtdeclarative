// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [document]
import QtQuick

Rectangle {
    width: 175; height: 350
    color: "lightgrey"

    Column {
        anchors.centerIn: parent
        spacing: 15
        Button {}
        Button {text: "Me Too!"}
        Button {text: "Me Three!"}
//! [grouped property]
        Button {label.color: "green"}
//! [grouped property]
    }
}
//! [document]
