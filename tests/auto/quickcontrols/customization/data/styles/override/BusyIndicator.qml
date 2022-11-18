// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
