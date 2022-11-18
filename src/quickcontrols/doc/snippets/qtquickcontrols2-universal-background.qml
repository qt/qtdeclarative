// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

Pane {
    padding: 10

//! [1]
Pane {
    Universal.background: Universal.Steel

    Button {
        text: qsTr("Button")
    }
}
//! [1]
}
