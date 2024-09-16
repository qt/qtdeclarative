// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.CheckBox {
    id: control
    objectName: "checkbox-simple"

    implicitWidth: contentItem.implicitWidth + indicator.implicitWidth
    implicitHeight: Math.max(contentItem.implicitHeight, indicator.implicitHeight)

    indicator: Text {
        objectName: "checkbox-indicator-simple"
        text: control.checked ? "V" : ""
    }

    contentItem: Text {
        objectName: "checkbox-contentItem-simple"
        text: control.text
    }
}
