// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    property int bad
    Component.a: {}
    Text {
        font.f: ""
    }

    property bool xxx: Component.a
}
