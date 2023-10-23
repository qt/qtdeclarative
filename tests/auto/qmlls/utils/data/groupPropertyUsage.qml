// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
}
