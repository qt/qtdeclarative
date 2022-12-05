// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.Page {
    id: control
    objectName: "page-override"

    contentItem: Item {
        objectName: "page-contentItem-override"
    }

    background: Item {
        objectName: "page-background-override"
    }
}
