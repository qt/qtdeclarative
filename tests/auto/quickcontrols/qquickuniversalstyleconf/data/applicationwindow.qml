// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

ApplicationWindow {
    width: 400
    height: 400

    property alias label: label

    Label {
        id: label
    }
}
