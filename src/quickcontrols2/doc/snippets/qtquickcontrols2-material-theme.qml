// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Pane {
    padding: 10

//! [1]
Pane {
    Material.theme: Material.Dark

    Button {
        text: qsTr("Button")
    }
}
//! [1]
}
