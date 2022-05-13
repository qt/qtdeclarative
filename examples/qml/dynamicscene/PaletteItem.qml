// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "itemCreation.js" as Code

Image {
    id: paletteItem

    property string componentFile
    property string image

    source: image

    MouseArea {
        anchors.fill: parent

        onPressed: (mouse)=> Code.startDrag(mouse);
        onPositionChanged: (mouse)=> Code.continueDrag(mouse);
        onReleased: (mouse)=> Code.endDrag(mouse);
    }
}
