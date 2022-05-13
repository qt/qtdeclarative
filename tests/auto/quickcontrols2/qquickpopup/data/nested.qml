// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
    }

    Popup {
        id: modelessPopup
        modal: false
        width: 100
        height: 100
    }
}
