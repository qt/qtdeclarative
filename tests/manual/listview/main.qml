/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Window 2.2
import QtQuick.Controls 2.5
import Qt.labs.qmlmodels 1.0
import TestModel 0.1

Window {
    id: root
    visible: true
    width: 640
    height: 480

    property int rows: 500
    property int columns: 20
    property real delegateHeight: 10
    property real delegateWidth: 50

    CheckBox {
        id: reuseItemsBox
        text: "Reuse items"
        checked: true
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 10
        anchors.topMargin: reuseItemsBox.height + 10
        color: "lightgray"

        ListView {
            id: listView
            anchors.fill: parent

            reuseItems: reuseItemsBox.checked

            cacheBuffer: 0
            contentWidth: columns * delegateWidth
            contentHeight: rows * delegateHeight
            flickableDirection: Flickable.HorizontalAndVerticalFlick
            clip: true

            model: TestModel {
                rowCount: root.rows
            }

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AlwaysOn }

            delegate: DelegateChooser {
                role: "delegateType"

                DelegateChoice {
                    roleValue: "type1"

                    Item {
                        width: listView.contentWidth
                        height: delegateHeight
                        property int reusedCount: 0

                        ListView.onReused: reusedCount++

                        Text {
                            id: text1
                            text: "Reused count:" + reusedCount
                            font.pixelSize: 9
                        }

                        Row {
                            id: choice1
                            width: listView.contentWidth
                            height: delegateHeight
                            anchors.left: text1.right
                            anchors.leftMargin: 5
                            property color color: Qt.rgba(0.6, 0.6, 0.8, 1)

                            Component.onCompleted: {
                                for (var i = 0; i < columns; ++i)
                                    cellComponent.createObject(choice1, {column: i, color: color})
                            }
                        }
                    }
                }

                DelegateChoice {
                    roleValue: "type2"

                    Item {
                        width: listView.contentWidth
                        height: delegateHeight
                        property int reusedCount: 0

                        ListView.onReused: reusedCount++

                        Text {
                            id: text2
                            text: "Reused count:" + reusedCount
                            font.pixelSize: 9
                        }

                        Row {
                            id: choice2
                            width: listView.contentWidth
                            height: delegateHeight
                            anchors.left: text2.right
                            anchors.leftMargin: 5
                            property color color: Qt.rgba(0.3, 0.3, 0.8, 1)

                            Component.onCompleted: {
                                for (var i = 0; i < columns; ++i)
                                    cellComponent.createObject(choice2, {column: i, color: color})
                            }
                        }
                    }
                }
            }

        }
    }

    Component {
        id: cellComponent
        Rectangle {
            height: delegateHeight
            width: delegateWidth
            property int column
            Text {
                text: "Lorem ipsum dolor sit amet"
                font.pixelSize: 9
            }
        }
    }
}
