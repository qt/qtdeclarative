// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 400
    height: 400
    title: "releaseAfterExitTransition"

    property alias popup: popup
    property alias modalPopup: modalPopup

    Popup {
        id: popup
        y: parent.height - height
        width: 50
        height: 50
        popupType: Popup.Item
    }

    Popup {
        id: modalPopup
        modal: true
        y: parent.height - height
        width: 50
        height: 50
        popupType: Popup.Item
        exit:  Transition { PauseAnimation { duration: 100 } }
    }
}
