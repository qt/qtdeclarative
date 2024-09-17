// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
//![0]
    Text { text: qsTr("hello") }
//![0]

//![1]
    // Translates the source text into the correct
    // plural form and replaces %n with the value of total.
    Text {
        text: qsTr("%n message(s) saved", "", total)
    }
//![1]
}
