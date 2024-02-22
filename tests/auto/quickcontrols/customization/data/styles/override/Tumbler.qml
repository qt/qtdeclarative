// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.Tumbler {
    id: control
    objectName: "tumbler-override"

    contentItem: ListView {
        objectName: "tumbler-contentItem-override"
    }

    background: Item {
        objectName: "tumbler-background-override"
    }
}
