// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: 400
    height: 400

    property alias drawer: drawer

    Drawer {
        id: drawer
        width: 200
        height: 200
    }
}
