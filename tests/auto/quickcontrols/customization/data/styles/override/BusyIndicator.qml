// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.BusyIndicator {
    id: control
    objectName: "busyindicator-override"

    contentItem: Item {
        objectName: "busyindicator-contentItem-override"
    }

    background: Item {
        objectName: "busyindicator-background-override"
    }
}
