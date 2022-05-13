// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
