/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
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

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl
import QtQuick.Layouts

GridLayout {
    property alias familyListView: fontFamilyListView
    property alias styleListView: fontStyleListView
    property alias sizeListView: fontSizeListView
    property alias sampleEdit: fontSample
    property alias underline: fontUnderline
    property alias strikeout: fontStrikeout
    property alias familyEdit: fontFamilyEdit
    property alias styleEdit: fontStyleEdit
    property alias sizeEdit: fontSizeEdit

    columns: 3

    ColumnLayout {
        spacing: 0

        Layout.preferredWidth: 50

        Label {
            text: qsTr("Family")
            Layout.alignment: Qt.AlignLeft
        }
        TextField {
            id: fontFamilyEdit
            objectName: "familyEdit"
            readOnly: true
            Layout.fillWidth: true
            focus: true
        }
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            background: Rectangle {
                color: "white"
            }
            ListView {
                id: fontFamilyListView
                objectName: "familyListView"
                implicitHeight: 200
                anchors.fill: parent
                clip: true

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                }

                boundsBehavior: Flickable.StopAtBounds

                highlightMoveVelocity: -1
                highlightMoveDuration: 1
                highlightFollowsCurrentItem: true
                keyNavigationEnabled: true

                delegate: ItemDelegate {
                    width: ListView.view.width
                    highlighted: ListView.isCurrentItem
                    onClicked: () => fontFamilyListView.currentIndex = index
                    text: modelData
                }
            }
        }
    }

    ColumnLayout {
        spacing: 0

        Layout.preferredWidth: 30

        Label {
            text: qsTr("Style")
            Layout.alignment: Qt.AlignLeft
        }
        TextField {
            id: fontStyleEdit
            objectName: "styleEdit"
            readOnly: true
            Layout.fillWidth: true
        }
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            background: Rectangle {
                color: "white"
            }
            ListView {
                id: fontStyleListView
                objectName: "styleListView"
                implicitHeight: 200
                anchors.fill: parent
                clip: true

                ScrollBar.vertical: ScrollBar {}
                boundsBehavior: Flickable.StopAtBounds

                highlightMoveVelocity: -1
                highlightMoveDuration: 1
                highlightFollowsCurrentItem: true
                keyNavigationEnabled: true

                delegate: ItemDelegate {
                    width: ListView.view.width
                    highlighted: ListView.isCurrentItem
                    onClicked: () => fontStyleListView.currentIndex = index
                    text: modelData
                }
            }
        }
    }

    ColumnLayout {
        spacing: 0

        Layout.preferredWidth: 20

        Label {
            text: qsTr("Size")
            Layout.alignment: Qt.AlignLeft
        }
        TextField {
            id: fontSizeEdit
            objectName: "sizeEdit"
            Layout.fillWidth: true
            validator: IntValidator {
                bottom: 1
                top: 512
            }
        }
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true

            background: Rectangle {
                color: "white"
            }
            ListView {
                id: fontSizeListView
                objectName: "sizeListView"
                implicitHeight: 200
                anchors.fill: parent
                clip: true

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                }

                boundsBehavior: Flickable.StopAtBounds

                highlightMoveVelocity: -1
                highlightMoveDuration: 1
                highlightFollowsCurrentItem: true
                keyNavigationEnabled: true

                delegate: ItemDelegate {
                    width: ListView.view.width
                    highlighted: ListView.isCurrentItem
                    onClicked: () => fontSizeListView.currentIndex = index
                    text: modelData
                }
            }
        }
    }

    ColumnLayout {
        Layout.preferredWidth: 80

        GroupBox {
            id: effectsGroupBox
            title: qsTr("Effects")

            Layout.fillWidth: true
            Layout.fillHeight: true

            label: Label {
                anchors.left: effectsGroupBox.left
                text: parent.title
            }

            RowLayout {
                anchors.fill: parent
                CheckBox {
                    id: fontUnderline
                    objectName: "underlineEffect"
                    text: qsTr("Underline")
                }
                CheckBox{
                    id: fontStrikeout
                    objectName: "strikeoutEffect"
                    text: qsTr("Strikeout")
                }
            }
        }
    }

    GroupBox {
        id: sample
        padding: label.implicitHeight
        title: qsTr("Sample")

        Layout.fillWidth: true
        Layout.preferredWidth: 80
        Layout.fillHeight: true
        Layout.columnSpan: 2
        clip: true

        background: Rectangle {
            y: sample.topPadding - sample.bottomPadding
            width: parent.width - sample.leftPadding + sample.rightPadding
            height: parent.height - sample.topPadding + sample.bottomPadding
            radius: 3
        }

        label: Label {
            anchors.left: sample.left
            text: sample.title
        }

        TextEdit {
            id: fontSample
            objectName: "sampleEdit"
            anchors.centerIn: parent
            readOnly: true
        }
    }
}
