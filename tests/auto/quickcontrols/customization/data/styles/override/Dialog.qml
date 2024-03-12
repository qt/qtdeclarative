// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import "../simple" as Simple

Simple.Dialog {
    id: control
    objectName: "dialog-override"

    contentItem: Item {
        objectName: "dialog-contentItem-override"
    }

    background: Item {
        objectName: "dialog-background-override"
    }
}
