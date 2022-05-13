// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.7
import QtQuick.Layouts 1.3

Rectangle {
    width: 100
    height: 100
    color: "black"

    property alias layout: layout
    property alias item1: r1

    RowLayout {
        id: layout
        anchors.fill: parent
        visible: false
        spacing: 0

        Rectangle {
            id: r1
            color: "red"

            layer.enabled: true

            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
