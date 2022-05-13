// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Switch {
    id: control
    objectName: "switch-simple"

    implicitWidth: contentItem.implicitWidth + indicator.implicitWidth
    implicitHeight: Math.max(contentItem.implicitHeight, indicator.implicitHeight)

    indicator: Item {
        objectName: "switch-indicator-simple"
    }

    contentItem: Item {
        objectName: "switch-contentItem-simple"
    }
}
