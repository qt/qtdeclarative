// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.TextArea {
    id: control
    objectName: "textarea-override"

    background: Item {
        objectName: "textarea-background-override"
    }
}
