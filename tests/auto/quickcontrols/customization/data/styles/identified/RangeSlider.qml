// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.RangeSlider {
    id: control
    objectName: "rangeslider-identified"

    first.handle: Item {
        id: firstHandle
        objectName: "rangeslider-first.handle-identified"
    }

    second.handle: Item {
        id: secondHandle
        objectName: "rangeslider-second.handle-identified"
    }

    background: Item {
        id: background
        objectName: "rangeslider-background-identified"
    }
}
