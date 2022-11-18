// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

Item {
    width: 360
    height: childrenRect.height

//! [1]
TabBar {
    width: parent.width
    TabButton {
        text: "First"
        width: implicitWidth
    }
    TabButton {
        text: "Second"
        width: implicitWidth
    }
    TabButton {
        text: "Third"
        width: implicitWidth
    }
}
//! [1]
}
