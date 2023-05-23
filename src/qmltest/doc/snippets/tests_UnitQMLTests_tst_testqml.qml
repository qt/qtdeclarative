// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
//! [import]
import QtQuick
import QtQuick.Controls

import QtTest
import MyModule

Item {
    width: 800; height: 600

    MyButton {
        id: myButton
        anchors.centerIn: parent
    }

    TestCase {
        name: "MyButton";
        when: windowShown;

        function test_clickToExpand() {
            const widthBeforeClick = myButton.width;
            mouseClick(myButton);
            const widthAfterClick = myButton.width;
            verify(widthBeforeClick < widthAfterClick);
        }
    }
}
//! [import]
