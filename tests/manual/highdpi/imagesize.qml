// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    width: 400
    height: 400

    Column {
        anchors.centerIn: parent.Center
        Row {
            Image {  width: 50; height: 50; fillMode: Image.Stretch; source : "heart-lowdpi.png" }
            Image {  width: 50; height: 50; fillMode: Image.Stretch; source : "heart.png" }
            Image {  width: 50; height: 50; fillMode: Image.Stretch; source : "heart-highdpi@2x.png" }
        }
        Row {
            Image {  width: 100; height: 100; fillMode: Image.Stretch; source : "heart-lowdpi.png" }
            Image {  width: 100; height: 100; fillMode: Image.Stretch; source : "heart.png" }
            Image {  width: 100; height: 100; fillMode: Image.Stretch; source : "heart-highdpi@2x.png" }
        }
        Row {
            Image {  width: 150; height: 150; fillMode: Image.Stretch; source : "heart-lowdpi.png" }
            Image {  width: 150; height: 150; fillMode: Image.Stretch; source : "heart.png" }
            Image {  width: 150; height: 150; fillMode: Image.Stretch; source : "heart-highdpi@2x.png" }
        }
        Row {
            Image {  width: 200; height: 200; fillMode: Image.Stretch; source : "heart-lowdpi.png" }
            Image {  width: 200; height: 200; fillMode: Image.Stretch; source : "heart.png" }
            Image {  width: 200; height: 200; fillMode: Image.Stretch; source : "heart-highdpi@2x.png" }
        }
        Row {
            Image {  width: 300; height: 300; fillMode: Image.Stretch; source : "heart-lowdpi.png" }
            Image {  width: 300; height: 300; fillMode: Image.Stretch; source : "heart.png" }
            Image {  width: 300; height: 300; fillMode: Image.Stretch; source : "heart-highdpi@2x.png" }
        }
    }
}
