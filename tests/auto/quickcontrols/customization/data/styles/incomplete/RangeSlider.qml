// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.RangeSlider {
    id: control
    objectName: "rangeslider-incomplete"

    first.handle: Item {
        objectName: "rangeslider-first.handle-incomplete"
    }

    second.handle: Item {
        objectName: "rangeslider-second.handle-incomplete"
    }

    background: Item {
        objectName: "rangeslider-background-incomplete"
    }
}
