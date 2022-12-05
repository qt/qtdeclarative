// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.TextField {
    id: control
    objectName: "textfield-override"

    background: Item {
        objectName: "textfield-background-override"
    }
}
