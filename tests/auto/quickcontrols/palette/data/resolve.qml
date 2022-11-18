// Copyright (C) 2022 zccrs <zccrs@live.com>, JiDe Zhang <zhangjide@uniontech.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    property alias control: control

    palette {window: "blue"; windowText: "white"}

    Control {
        id: control
    }

    function changeColors(color) {
        control.palette.window = color
        window.palette.windowText = color
    }
}
