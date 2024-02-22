// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import Stuff 1.0

Item {
    id: root

    Column {
        width: 100
        clip: true
        PerPixelRect { width: 100; height: 100; color: "red" }
        PerPixelRect { width: 100; height: 100; color: "blue" }
    }

    Column {
        x: 100
        width: 100
        PerPixelRect { width: 100; height: 100; color: "black" }
        PerPixelRect { width: 100; height: 100; color: "#00ff00" }
    }
}
