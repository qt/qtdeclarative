// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Text {
    id: textRoot

    Test {
        id: test
        myText {
            font {
                pixelSize: 12
                family: "serif"
            }
        }
    }

    component Test : Text{
        property Text myText
    }

    font.family: test.myText.font.family
    font {
        pixelSize: 12
    }

    Item {
        property var family // should not be in groupPropertyUsages1
        property int font // should not be in groupPropertyUsages2

        property var realFont: textRoot.font // should be in groupPropertyUsages2
        property var realFamily: textRoot.font.family // should be in groupPropertyUsages1
    }
}
