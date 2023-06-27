// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [document]
import QtQuick

Rectangle {
    width: 112; height: 112
    color: "#303030"

    Grid {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        columns: 2
        spacing: 6

        Rectangle { color: "#aa6666"; width: 50; height: 50 }
        Rectangle { color: "#aaaa66"; width: 50; height: 50 }
        Rectangle { color: "#9999aa"; width: 50; height: 50 }
        Rectangle { color: "#6666aa"; width: 50; height: 50 }
    }
}
//! [document]
