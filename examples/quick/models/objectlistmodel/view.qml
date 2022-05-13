// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

//![0]
ListView {
    id: listview
    width: 200; height: 320
    required model
    ScrollBar.vertical: ScrollBar { }

    delegate: Rectangle {
        width: listview.width; height: 25

        required color
        required property string name

        Text { text: parent.name }
    }
}
//![0]
