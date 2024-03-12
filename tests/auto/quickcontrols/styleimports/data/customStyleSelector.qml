// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    title: "Test Application Window"
    width: 400
    height: 400

    property alias customComponent: customComponent

    CustomComponent {
        id: customComponent
    }
}
