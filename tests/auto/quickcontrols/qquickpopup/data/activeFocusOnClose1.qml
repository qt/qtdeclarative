// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias focusedPopup: focusedPopup
    property alias nonFocusedPopup: nonFocusedPopup

    Popup {
        id: focusedPopup
        focus: true
    }

    Popup {
        id: nonFocusedPopup
    }
}
