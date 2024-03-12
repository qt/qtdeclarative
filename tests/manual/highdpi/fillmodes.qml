// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    width: 400
    height: 700

    // the images should have the same display size and appearance on each row.
    Column {
        anchors.centerIn: parent.Center
        Row {
            Image {  width: 130; height: 70; fillMode: Image.Stretch; source : "heart-lowdpi.png" }
            Image {  width: 130; height: 70; fillMode: Image.Stretch; source : "heart.png" }
            Image {  width: 130; height: 70; fillMode: Image.Stretch; source : "heart-highdpi@2x.png" }
        }
        Row {
            Image {  width: 130; height: 100; fillMode: Image.PreserveAspectFit; source : "heart-lowdpi.png" }
            Image {  width: 130; height: 100; fillMode: Image.PreserveAspectFit; source : "heart.png" }
            Image {  width: 130; height: 100; fillMode: Image.PreserveAspectFit; source : "heart-highdpi@2x.png" }
        }
        Row {
            Image {  width: 130; height: 100; fillMode: Image.PreserveAspectCrop; source : "heart-lowdpi.png" }
            Image {  width: 130; height: 100; fillMode: Image.PreserveAspectCrop; source : "heart.png" }
            Image {  width: 130; height: 100; fillMode: Image.PreserveAspectCrop; source : "heart-highdpi@2x.png" }
        }
        Row {
            Image {  width: 230; height: 200; fillMode: Image.Tile; source : "heart-lowdpi.png" }
            Image {  width: 230; height: 200; fillMode: Image.Tile; source : "heart.png" }
            Image {  width: 230; height: 200; fillMode: Image.Tile; source : "heart-highdpi@2x.png" }
        }
    }
}
