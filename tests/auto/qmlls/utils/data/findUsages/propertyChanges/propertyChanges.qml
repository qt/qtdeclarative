// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
Item {
    MouseArea {
        id: mouse11
        onClicked: doSomething()
        property rect r
    }

    states: [
        State {
            PropertyChanges {
                mouse11 { // block notation
                    onClicked: doSomethingElse()
                    r : 34
                }
                mouse11.onClicked: doSomething(); // dot notation
            }

            // with target property
            PropertyChanges {
                target: mouse11
                onClicked: doSomethingElse()
                r: 45
            }
        }
    ]
    function doSomething() {}
    function doSomethingElse() {}

    property rect r // shouldn't be found
    property int clicked // shouldn't be gound
}
