// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias stackView: stackView
    property alias nextItem: nextItem

    function pushNextItem() {
        stackView.push(nextItem, StackView.Immediate);
    }

    Rectangle {
        id: nextItem
        color: "blue"
        visible: false
    }

    StackView {
        id: stackView
        anchors.fill: parent

        initialItem: Rectangle {
            color: "red"
        }
    }
}
