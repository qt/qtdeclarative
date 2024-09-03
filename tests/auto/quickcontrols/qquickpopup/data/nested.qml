// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias modalPopup: modalPopup
    property alias modelessPopup: modelessPopup

    Popup {
        id: modalPopup
        modal: true
        width: 200
        height: 200
        popupType: Popup.Item
    }

    Popup {
        id: modelessPopup
        modal: false
        width: 100
        height: 100
        popupType: Popup.Item
    }
}
