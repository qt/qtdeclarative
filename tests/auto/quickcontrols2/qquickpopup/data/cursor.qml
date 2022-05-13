// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias popup: popup
    property alias textField: textField

    TextField {
        id: textField
    }
    Popup {
        id: popup
        x: textField.x + textField.width / 2
        y: textField.y + textField.height / 2 - height / 2
        width: 100
        height: 100
    }
}
