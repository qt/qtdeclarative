// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
//![0]

ListView {
    width: 100
    height: 100
    required model

    delegate: Rectangle {
        required property string modelData
        height: 25
        width: 100
        Text { text: parent.modelData }
    }
}
//![0]
