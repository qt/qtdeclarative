// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400
    visible: true

    HorizontalHeaderView {
        id: hh
        objectName: "horizontalHeader"
        implicitWidth: 40
        model: 3
    }
}
