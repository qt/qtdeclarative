// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    width: 400
    height: 400

    property alias label: label
    property alias button: button

    Label {
        id: label
    }

    Button {
        id: button
    }
}
