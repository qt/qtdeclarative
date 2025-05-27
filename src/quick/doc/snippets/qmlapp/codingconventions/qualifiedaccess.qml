// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Item {
    id: root

    property int rectangleWidth: 50

    Rectangle {
        width: root.rectangleWidth
    }
}
//![0]
