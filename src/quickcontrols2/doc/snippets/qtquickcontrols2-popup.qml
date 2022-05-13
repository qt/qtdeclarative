// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

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
