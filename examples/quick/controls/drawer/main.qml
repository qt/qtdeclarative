/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2
import QtQuick.Controls 2.0
import QtQuick.Extras 2.0

ApplicationWindow {
    id: window
    width: 360
    height: 520
    visible: true
    title: "Qt Quick Controls - Drawer Example"

    Rectangle {
        id: content
        anchors.fill: parent
        anchors.margins: -1
        border.color: window.style.frameColor

        Image {
            width: window.width / 2
            height: window.height / 2
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: window.width > window.height ? width / 2 : 0
            anchors.verticalCenterOffset: window.width < window.height ? -height / 4 : 0
            fillMode: Image.PreserveAspectFit
            source: "qrc:/images/qt-logo.png"
        }

        Image {
            width: window.width / 2
            anchors.bottom: parent.bottom
            anchors.bottomMargin: height / 2
            fillMode: Image.PreserveAspectFit
            source: "qrc:/images/arrow.png"
        }

        transform: Translate {
            x: effect.current === uncover ? drawer.position * listview.width :
               effect.current === push ? drawer.position * listview.width * 0.5 : 0
        }

        z: effect.current === uncover ? 2 : 0
    }

    Drawer {
        id: drawer
        anchors.fill: parent

        ListView {
            id: listview

            width: window.width / 3 * 2
            height: window.height

            ExclusiveGroup {
                id: effect
            }

            model: VisualItemModel {
                Label {
                    text: "Settings"
                    x: window.style.padding
                    width: parent.width - window.style.padding * 2
                    lineHeight: 2.0
                    color: window.style.accentColor
                    verticalAlignment: Text.AlignVCenter
                }
                Rectangle { width: parent.width; height: 1; color: window.style.frameColor }
                Switch {
                    id: dim
                    text: "Dim"
                    checked: true
                    width: parent.width
                    layoutDirection: Qt.RightToLeft
                    enabled: effect.current != uncover
                }
                Rectangle { width: parent.width; height: 1; color: window.style.frameColor }
                RadioButton {
                    id: overlay
                    text: "Overlay"
                    checked: true
                    width: parent.width
                    Exclusive.group: effect
                    layoutDirection: Qt.RightToLeft
                }
                RadioButton {
                    id: push
                    text: "Push"
                    width: parent.width
                    Exclusive.group: effect
                    layoutDirection: Qt.RightToLeft
                }
                RadioButton {
                    id: uncover
                    text: "Uncover"
                    width: parent.width
                    Exclusive.group: effect
                    layoutDirection: Qt.RightToLeft
                }
                Rectangle { width: parent.width; height: 1; color: window.style.frameColor }
            }
            Rectangle {
                z: -1
                anchors.fill: parent
                anchors.topMargin: -1
                anchors.bottomMargin: -1
                border.color: window.style.frameColor
            }

            transform: Translate {
                x: effect.current === uncover ? (1.0 - drawer.position) * listview.width : 0
            }
        }

        background.visible: dim.checked

        onClicked: close()
    }
}
