// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls.Universal

MouseArea {
    id: delegateRoot
    signal songSelected(albumName: string, albumArt: string, artistName: string, songTitle: string)

    implicitHeight: 60
    enabled: model.albumArt !== ""
    onClicked: songSelected(model.album, model.albumArt, model.artist, model.song)
    BusyIndicator {
        id: loadingIndicator
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: loadingText.left
        anchors.rightMargin: 5
        height: loadingText.implicitHeight
        width: height
        running: model.loadingText === "" && model.isLoadingElement
    }
    Text {
        id: loadingText
        anchors.centerIn: parent
        font.pixelSize: delegateRoot.implicitHeight / 4
        text: model.loadingText === "" ? qsTr("Loading...") : model.loadingText
        visible: model.isLoadingElement
    }
    Item {
        anchors.fill: parent
        visible: !loadingIndicator.running
        Image {
            id: albumArtImage
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 1
            anchors.topMargin: 1
            anchors.bottomMargin: 1
            source: model.albumArt
            width: height
            fillMode: Image.PreserveAspectCrop
        }
        Text {
            id: artistName
            anchors.left: albumArtImage.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.leftMargin: 1
            anchors.rightMargin: 1
            font.pixelSize: delegateRoot.implicitHeight / 5
            text: model.artist
        }
        Text {
            id: songName
            anchors.left: albumArtImage.right
            anchors.right: parent.right
            anchors.top: artistName.bottom
            anchors.topMargin: 1
            anchors.leftMargin: 1
            anchors.rightMargin: 1
            font.pixelSize: delegateRoot.implicitHeight / 4
            text: model.song
        }
        Text {
            id: albumName
            anchors.left: albumArtImage.right
            anchors.right: parent.right
            anchors.top: songName.bottom
            anchors.topMargin: 1
            anchors.leftMargin: 1
            anchors.rightMargin: 1
            font.pixelSize: delegateRoot.implicitHeight / 5
            text: model.album
        }
    }
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        implicitHeight: 1
        color: "lightgray"
    }
    Text {
        anchors.right: parent.right
        anchors.rightMargin: delegateRoot.implicitHeight / 2
        anchors.verticalCenter: parent.verticalCenter
        font.pixelSize: delegateRoot.implicitHeight / 4
        text: "#"+(index+1)
    }
}
