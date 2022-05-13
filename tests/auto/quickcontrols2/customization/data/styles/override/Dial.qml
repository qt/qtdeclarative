// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.Dial {
    id: control
    objectName: "dial-override"

    handle: Item {
        objectName: "dial-handle-override"
    }

    background: Item {
        objectName: "dial-background-override"
    }
}
