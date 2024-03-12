// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Slider {
    id: control
    objectName: "slider-identified"

    handle: Item {
        id: handle
        objectName: "slider-handle-identified"
    }

    background: Item {
        id: background
        objectName: "slider-background-identified"
    }
}
