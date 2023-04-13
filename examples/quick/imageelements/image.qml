// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQuick

Rectangle {
    width: 320
    height: 480
    Grid {
        id: grid

        property int cellWidth: (width - (spacing * (columns - 1))) / columns
        property int cellHeight: (height - (spacing * (rows - 1))) / rows

        anchors.fill: parent
        anchors.margins: 30

        columns: 2
        rows: 3
        spacing: 30

        component SizedImageCell: ImageCell {
            width: grid.cellWidth
            height: grid.cellHeight
        }

        SizedImageCell {
            mode: Image.Stretch
            caption: "Stretch"
        }
        SizedImageCell {
            mode: Image.PreserveAspectFit
            caption: "PreserveAspectFit"
        }
        SizedImageCell {
            mode: Image.PreserveAspectCrop
            caption: "PreserveAspectCrop"
        }
        SizedImageCell {
            mode: Image.Tile
            caption: "Tile"
        }
        SizedImageCell {
            mode: Image.TileHorizontally
            caption: "TileHorizontally"
        }
        SizedImageCell {
            mode: Image.TileVertically
            caption: "TileVertically"
        }
    }
}
