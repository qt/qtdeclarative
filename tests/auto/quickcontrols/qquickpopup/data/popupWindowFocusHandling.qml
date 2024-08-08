// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Window {
    width: 400
    height: 400

    property alias popup: simplepopup
    property alias textField1: outerTextField
    property alias textField2: innerTextField

    TextField {
        id: outerTextField
        focus: true
    }

    Popup {
        id: simplepopup
        popupType: Popup.Window
        focus: true
        TextField {
            id: innerTextField
            focus: true
        }
    }
}
