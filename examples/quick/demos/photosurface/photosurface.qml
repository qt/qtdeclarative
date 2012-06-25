/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
import QtQuick 2.0
import QtQuick.Dialogs 1.0
import Qt.labs.folderlistmodel 1.0

Rectangle {
    id: root
    width: 1024; height: 600
    color: "black"
    visible: true
    property int highestZ: 0
    property real defaultSize: 200

    FileDialog {
        id: fileDialog
        title: "Choose a folder with some images"
        selectFolder: true
        onAccepted: { folderModel.folder = filePaths[0] + "/" }
    }

    Repeater {
        model: FolderListModel {
            id: folderModel
            objectName: "folderModel"
            showDirs: false
            nameFilters: ["*.png", "*.jpg", "*.gif"]
        }
        Rectangle {
            id: photoFrame
            width: image.width * image.scale + 20
            height: image.height * image.scale + 20
            border.color: "black"
            border.width: 2
            smooth: true
            antialiasing: true
            x: Math.random() * root.width - defaultSize
            y: Math.random() * root.height - defaultSize
            rotation: Math.random() * 13 - 6
            Image {
                id: image
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
                source: folderModel.folder + fileName
                scale: defaultSize / Math.max(sourceSize.width, sourceSize.height)
                antialiasing: true
            }
            PinchArea {
                anchors.fill: parent
                property real initialScale
                property real initialRotation
                onPinchStarted: {
                    initialScale = image.scale
                    initialRotation = parent.rotation
                }
                onPinchUpdated: {
                    image.scale = initialScale * pinch.scale;
                    parent.rotation = initialRotation + pinch.rotation
                    if (Math.abs(photoFrame.rotation) < 4)
                        parent.rotation = 0;
                }
                onPinchFinished: photoFrame.border.color = "black";
                MouseArea {
                    id: dragArea
                    hoverEnabled: true
                    anchors.fill: parent
                    drag.target: photoFrame
                    onPressed: photoFrame.z = ++root.highestZ;
                    onEntered: photoFrame.border.color = "red";
                    onExited: photoFrame.border.color = "black";
                    onWheel: {
                        if (wheel.modifiers & Qt.ControlModifier) {
                            photoFrame.rotation += wheel.angleDelta.y / 120 * 5;
                            if (Math.abs(photoFrame.rotation) < 4)
                                photoFrame.rotation = 0;
                        } else {
                            photoFrame.rotation += wheel.angleDelta.x / 120;
                            if (Math.abs(photoFrame.rotation) < 0.6)
                                photoFrame.rotation = 0;
                            var scaleBefore = image.scale;
                            image.scale += image.scale * wheel.angleDelta.y / 120 / 10;
                            photoFrame.x -= image.width * (image.scale - scaleBefore) / 2.0;
                            photoFrame.y -= image.height * (image.scale - scaleBefore) / 2.0;
                        }
                    }
                }
            }
        }
    }
    Text {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 10
        color: "darkgrey"
        text: "On a touchscreen: use two fingers to zoom and rotate, one finger to drag\n" +
              "With a mouse: drag normally, use the vertical wheel to zoom, horizontal wheel to rotate, or hold Ctrl while using the vertical wheel to rotate"
    }

    Component.onCompleted: fileDialog.open()
}
