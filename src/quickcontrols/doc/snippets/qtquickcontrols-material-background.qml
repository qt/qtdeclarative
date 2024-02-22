// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
