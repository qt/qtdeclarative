// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.SwipeDelegate {
    id: control
    objectName: "swipedelegate-override"

    contentItem: Item {
        objectName: "swipedelegate-contentItem-override"
    }

    background: Item {
        objectName: "swipedelegate-background-override"
    }
}
