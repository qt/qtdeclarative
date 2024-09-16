// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Text {
    id: grouped

    // Dot notation
    Binding {
        grouped.font.family: "mono"
    }

    // Group notation
    Test {
        id: test
        myText {
            font {
                pixelSize: 12
            }
        }
    }

    component Test : Rectangle {
        property Text myText: text1
        Text {
            id: text1
        }
    }

    Keys.onPressed: (event)=> {

    }
}
