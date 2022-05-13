// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Pane {
    padding: 4

//! [1]
Button {
    text: qsTr("Button")
    highlighted: true
    Material.background: Material.Teal
}
//! [1]
}
