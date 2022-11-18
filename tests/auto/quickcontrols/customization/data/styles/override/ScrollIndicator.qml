// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "../simple" as Simple

Simple.ScrollIndicator {
    id: control
    objectName: "scrollindicator-override"

    contentItem: Item {
        objectName: "scrollindicator-contentItem-override"
    }

    background: Item {
        objectName: "scrollindicator-background-override"
    }
}
