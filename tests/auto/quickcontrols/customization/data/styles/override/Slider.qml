// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.Slider {
    id: control
    objectName: "slider-override"

    handle: Item {
        objectName: "slider-handle-override"
    }

    background: Item {
        objectName: "slider-background-override"
    }
}
