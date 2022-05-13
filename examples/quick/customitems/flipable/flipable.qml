// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: window

    width: 480; height: 320
    color: "darkgreen"

    Row {
        anchors.centerIn: parent; spacing: 30
        Card { source: "9_club.png"; angle: 180; yAxis: 1 }
        Card { source: "5_heart.png"; angle: 540; xAxis: 1 }
    }
}
