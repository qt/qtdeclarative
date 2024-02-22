// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 200
    height: 200
    visible: true

    header: RowLayout {
        Rectangle { color: "red"; implicitWidth: 20; implicitHeight: 20; Layout.fillWidth: true}
    }
    footer: ColumnLayout {
        Rectangle { color: "green"; implicitWidth: 20; implicitHeight: 20; Layout.fillWidth: true}
    }
}
