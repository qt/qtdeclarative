// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
