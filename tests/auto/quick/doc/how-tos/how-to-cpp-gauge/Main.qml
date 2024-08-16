// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//! [file]
import QtQuick.Controls

import GaugeHowTo

ApplicationWindow {
    width: 400
    height: 400
    title: qsTr("Gauge example")

    Gauge {
        minimumValue: 0
        value: 75
        maximumValue: 100
    }
}
//! [file]
