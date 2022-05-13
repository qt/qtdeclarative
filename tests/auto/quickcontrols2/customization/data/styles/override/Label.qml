// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.Label {
    id: control
    objectName: "label-override"

    background: Item {
        objectName: "label-background-override"
    }
}
