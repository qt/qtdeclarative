// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.StackView {
    id: control
    objectName: "stackview-override"

    contentItem: Item {
        objectName: "stackview-contentItem-override"
    }

    background: Item {
        objectName: "stackview-background-override"
    }
}
