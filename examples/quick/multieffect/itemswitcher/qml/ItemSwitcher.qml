// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: rootItem

    property var sourceItems: []

    property Item fromItem
    property Item toItem

    property var effect

    property int currentIndex: 0
    property int previousIndex: 0
    property real inAnimation: 0
    readonly property real outAnimation: 1.0 - inAnimation
    // Duration of switch animation, in ms
    property int duration: 1500

    property bool _initialized: false

    onCurrentIndexChanged: {
        fromItem = sourceItems[previousIndex];
        toItem = sourceItems[currentIndex];
        if (_initialized)
            switchAnimation.restart();
        previousIndex = currentIndex;
    }

    // Initialize the items and currentIndex
    Timer {
        running: true
        interval: 0
        onTriggered: {
            fromItem = sourceItems[previousIndex];
            toItem = sourceItems[currentIndex];
            previousIndex = currentIndex;
            _initialized = true;
        }
    }

    SequentialAnimation {
        id: switchAnimation
        alwaysRunToEnd: true
        NumberAnimation {
            target: rootItem
            property: "inAnimation"
            from: 0
            to: 1
            duration: rootItem.duration
            easing.type: Easing.InOutQuad
        }
    }
}
