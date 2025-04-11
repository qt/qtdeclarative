// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

//! [Fixed Controls style]
import QtQuick
import QtQuick.Controls.Universal
//! [Fixed Controls style]

Window {
    id: mainWindow
    minimumWidth: 800
    minimumHeight: 480
    width: minimumWidth
    height: minimumHeight
    visible: true
    title: qsTr("Threaded Song List")

    property string currentAlbumName: ""
    property string currentAlbumArt: ""
    property string currentArtistName: ""
    property string currentSongTitle: ""
    ListView {
        id: songListView
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 2 * parent.width / 5
        model: ThreadedListModel{}
        readonly property int delegateHeight: (songListView.height / 8) - songListView.spacing
        cacheBuffer: delegateHeight * 2
        delegate: SongListDelegate{
            required property int index
            required property var model
            implicitHeight: songListView.delegateHeight
            width: songListView.width
            onSongSelected: (albumName, albumArt, artistName, songTitle) => {
                mainWindow.currentAlbumName = albumName
                mainWindow.currentAlbumArt = albumArt
                mainWindow.currentArtistName = artistName
                mainWindow.currentSongTitle = songTitle
            }
        }
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
            implicitWidth: songListView.height / 20
        }
        BusyIndicator {
            anchors.centerIn: parent
            width: parent.width / 3
            height: width
            running: songListView.count === 0
        }
    }
    Rectangle {
        color: "white"
        anchors.left: songListView.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        Label {
            id: statusLabel
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.topMargin: 5
            textFormat: Text.RichText
            text: qsTr("&#x1F517; Connected, %1 songs available").arg(songListView.count)
            font.pixelSize: Math.min(parent.height, parent.width) / 32
            horizontalAlignment: Text.AlignHCenter
        }
        Item {
            id: deviceImage
            anchors.right: parent.right
            anchors.top: statusLabel.bottom
            anchors.left: parent.left
            anchors.leftMargin: 10
            height: (3 * parent.height / 4) - statusLabel.height
            Image {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(parent.width, parent.height)
                height: width
                source: "qrc:/qt/qml/threadedsonglist/images/device/remote.jpeg"
                fillMode: Image.PreserveAspectFit
            }
        }
        Rectangle {
            visible: mainWindow.currentSongTitle !== ""
            color: "black"
            anchors.top: deviceImage.bottom
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            Rectangle {
                id: nowPlayingBox
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.left: parent.left
                height: nowPlayingTitle.implicitHeight
                color: "#555555"
                Label {
                    id: nowPlayingTitle
                    anchors.centerIn: parent
                    color: "white"
                    text: qsTr("Now playing:")
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                }
            }
            Image {
                id: albumArtImage
                anchors.top: nowPlayingBox.bottom
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.topMargin: 5
                source: mainWindow.currentAlbumArt
                fillMode: Image.PreserveAspectFit
            }
            Column {
                anchors.top: nowPlayingBox.bottom
                anchors.right: parent.right
                anchors.left: albumArtImage.right
                anchors.bottom: parent.bottom
                anchors.topMargin: 5
                anchors.leftMargin: 5
                spacing: 5
                Label {
                    color: "white"
                    text: "&#x266B;: " + mainWindow.currentSongTitle
                    textFormat: Text.RichText
                    font.pixelSize: (parent.height - 20) / 4
                }
                Label {
                    color: "white"
                    text: "&#x1f464;: " + mainWindow.currentArtistName
                    textFormat: Text.RichText
                    font.pixelSize: (parent.height - 20) / 4
                }
                Label {
                    color: "white"
                    text: "&#x1F4BF;: " + mainWindow.currentAlbumName
                    textFormat: Text.RichText
                    font.pixelSize: (parent.height - 20) / 4
                }
            }
        }
    }
}
