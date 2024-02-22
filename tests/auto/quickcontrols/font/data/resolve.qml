// Copyright (C) 2022 yeshanshan <yeshanshan@uniontech.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    property alias control1: control1
    property alias control2: control2
    property alias control2ChildControl: control3
    property alias control4: control4

    font {family: "Helvetica"; pointSize: 10; weight: 10}

    Control {
        id: control1
    }

    function changeFont1(font) {
        control1.font.pointSize = font.pointSize
        window.font.weight = font.weight
    }

    Control {
        id: control2

        Control {
            id: control3
            // The font's resolve mask of control4 should be is 0, the
            // controls3's font is same thing. because control3's font
            // resolve mask is 0 also, so the font's all value equal to
            // the parent font values.
            font: control4.font
        }
    }

    Control {
        font.pointSize: 10

        Control {
            id: control4
        }
    }

    function changeFont2(font) {
        control2.font.pointSize = font.pointSize
        control2.font.weight = font.weight
    }
}
