// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.Container {
    id: control
    objectName: "container-override"

    contentItem: Item {
        objectName: "container-contentItem-override"
    }

    background: Item {
        objectName: "container-background-override"
    }
}
