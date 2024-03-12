// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtCore
import QtQuick
import QtQuick.Dialogs
import Qt.labs.folderlistmodel
import "content"

Rectangle {
    id: root
    visible: true
    width: 1024; height: 600
    color: "black"
    property real defaultSize: 200
    property real surfaceViewportRatio: 1.5

    FolderDialog {
        id: folderDialog
        title: "Choose a folder with some images"
        onAccepted: folderModel.folder = selectedFolder + "/"
    }
    Shortcut {
        id: openShortcut
        sequence: StandardKey.Open
        onActivated: folderDialog.open()
    }

    FakeFlickable {
        id: flick
        anchors.fill: parent
        contentWidth: width * root.surfaceViewportRatio
        contentHeight: height * root.surfaceViewportRatio
        property int highestZ: 0
        Repeater {
            model: FolderListModel {
                id: folderModel
                objectName: "folderModel"
                showDirs: false
                nameFilters: ["*.png", "*.jpg", "*.gif"]
            }
            delegate: Rectangle {
                required property string fileModified
                required property string fileName
                required property string fileUrl
                id: photoFrame
                objectName: "frame-" + fileName
                width: image.width * (1 + 0.10 * image.height / image.width)
                height: image.height * 1.10
                scale: defaultSize / Math.max(image.sourceSize.width, image.sourceSize.height)
                Behavior on scale { NumberAnimation { duration: 200 } }
                Behavior on x { NumberAnimation { duration: 200 } }
                Behavior on y { NumberAnimation { duration: 200 } }
                border.color: pinchHandler.active || dragHandler.active ? "red" : "black"
                border.width: 2
                smooth: true
                antialiasing: true
                Component.onCompleted: {
                    x = Math.random() * root.width - width / 2
                    y = Math.random() * root.height - height / 2
                    rotation = Math.random() * 13 - 6
                }

                Image {
                    id: image
                    anchors.centerIn: parent
                    fillMode: Image.PreserveAspectFit
                    source: photoFrame.fileUrl
                    antialiasing: true
                }

                Text {
                    text: fileName + " ❖ " + Qt.formatDateTime(fileModified, Locale.LongFormat)
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    font.pixelSize: (parent.height - image.height) / 3
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                        margins: font.pixelSize / 5
                    }
                }

                MomentumAnimation {
                    id: anim
                    target: photoFrame
                    onFinished: {
                        flick.contentWidth = Math.max(photoFrame.x + photoFrame.width, flick.contentWidth)
                        flick.contentHeight = Math.max(photoFrame.y + photoFrame.height, flick.contentHeight)
                    }
                }

                PinchHandler {
                    id: pinchHandler
                    minimumRotation: -360
                    maximumRotation: 360
                    minimumScale: 0.1
                    maximumScale: 10
                    grabPermissions: PointerHandler.CanTakeOverFromAnything // and never gonna give it up
                    onActiveChanged: if (active) photoFrame.z = ++flick.highestZ
                }

                DragHandler {
                    id: dragHandler
                    onActiveChanged: {
                        if (active)
                            photoFrame.z = ++flick.highestZ
                        else
                            anim.restart(centroid.velocity)
                    }
                }
            }
        }
    }

    Rectangle {
        id: verticalScrollDecorator
        anchors.right: parent.right
        anchors.margins: 2
        color: "cyan"
        border.color: "black"
        border.width: 1
        width: 5
        radius: 2
        antialiasing: true
        height: flick.height * (flick.height / flick.contentHeight) - (width - anchors.margins) * 2
        y:  -flick.contentY * (flick.height / flick.contentHeight)
        NumberAnimation on opacity { id: vfade; to: 0; duration: 500 }
        onYChanged: { opacity = 1.0; fadeTimer.restart() }
    }

    Rectangle {
        id: horizontalScrollDecorator
        anchors.bottom: parent.bottom
        anchors.margins: 2
        color: "cyan"
        border.color: "black"
        border.width: 1
        height: 5
        radius: 2
        antialiasing: true
        width: flick.width * (flick.width / flick.contentWidth) - (height - anchors.margins) * 2
        x:  -flick.contentX * (flick.width / flick.contentWidth)
        NumberAnimation on opacity { id: hfade; to: 0; duration: 500 }
        onXChanged: { opacity = 1.0; fadeTimer.restart() }
    }

    Timer { id: fadeTimer; interval: 1000; onTriggered: { hfade.start(); vfade.start() } }

    Text {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        color: "darkgrey"
        wrapMode: Text.WordWrap
        font.pointSize: 8
        text: "Press " + openShortcut.nativeText + " to choose a different image folder\n" +
              "On a touchscreen: use two fingers to zoom and rotate, one finger to drag\n" +
              "With a mouse: drag normally"
    }

    Shortcut { sequence: StandardKey.Quit; onActivated: Qt.quit() }

    Component.onCompleted: {
        const lastArg = Application.arguments.slice(-1)[0]
        const standardPicturesLocations = StandardPaths.standardLocations(StandardPaths.PicturesLocation)
        const hasValidPicturesLocation = standardPicturesLocations.length > 0
        if (hasValidPicturesLocation)
            folderDialog.currentFolder = standardPicturesLocations[0]
        if (/.*hotosurface.*|--+/.test(lastArg)) {
            if (hasValidPicturesLocation)
                folderModel.folder = standardPicturesLocations[0]
            else
                folderDialog.open()
        }
        else
            folderModel.folder = Qt.resolvedUrl("file:" + lastArg)
    }
}
