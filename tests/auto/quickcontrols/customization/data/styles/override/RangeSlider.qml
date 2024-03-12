// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.RangeSlider {
    id: control
    objectName: "rangeslider-override"

    first.handle: Item {
        objectName: "rangeslider-first.handle-override"
    }

    second.handle: Item {
        objectName: "rangeslider-second.handle-override"
    }

    background: Item {
        objectName: "rangeslider-background-override"
    }
}
