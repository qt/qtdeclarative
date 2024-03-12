// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Item {
    visible: true
    width: 360
    height: 520

    ComboBox {
        model: ["First", "Second", "Third"]
        anchors.centerIn: parent
    }
}
