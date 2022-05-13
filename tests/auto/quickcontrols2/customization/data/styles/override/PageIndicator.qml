// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.PageIndicator {
    id: control
    objectName: "pageindicator-override"

    contentItem: Item {
        objectName: "pageindicator-contentItem-override"
    }

    background: Item {
        objectName: "pageindicator-background-override"
    }
}
