// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.Control {
    id: control
    objectName: "control-override"

    contentItem: Item {
        objectName: "control-contentItem-override"
    }

    background: Item {
        objectName: "control-background-override"
    }
}
