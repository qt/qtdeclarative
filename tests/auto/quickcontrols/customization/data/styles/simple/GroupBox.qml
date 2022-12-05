// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.GroupBox {
    id: control
    objectName: "groupbox-simple"

    implicitWidth: Math.max(background.implicitWidth, contentWidth)
    implicitHeight: Math.max(background.implicitHeight, contentHeight)

    contentWidth: contentItem.implicitWidth || (contentChildren.length === 1 ? contentChildren[0].implicitWidth : 0)
    contentHeight: contentItem.implicitHeight || (contentChildren.length === 1 ? contentChildren[0].implicitHeight : 0)

    contentItem: Item {
        objectName: "groupbox-contentItem-simple"
    }

    background: Rectangle {
        objectName: "groupbox-background-simple"
        implicitWidth: 20
        implicitHeight: 20
    }

    label: Text {
        objectName: "groupbox-label-simple"
        text: control.title
    }
}
