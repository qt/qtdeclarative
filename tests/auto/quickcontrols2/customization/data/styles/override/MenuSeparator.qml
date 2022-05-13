// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.MenuSeparator {
    id: control
    objectName: "menuseparator-override"

    contentItem: Item {
        objectName: "menuseparator-contentItem-override"
    }

    background: Item {
        objectName: "menuseparator-background-override"
    }
}
