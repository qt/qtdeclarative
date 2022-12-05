// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Dial {
    id: control
    objectName: "dial-identified"

    handle: Item {
        id: handle
        objectName: "dial-handle-identified"
    }

    background: Item {
        id: background
        objectName: "dial-background-identified"
    }
}
