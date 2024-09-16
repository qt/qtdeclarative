// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Item {
//! [centerIn]
ApplicationWindow {
    id: window
    // ...

    Pane {
        // ...

        Popup {
            anchors.centerIn: Overlay.overlay
        }
    }
}
//! [centerIn]
}
