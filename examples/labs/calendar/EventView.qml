/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.6
import Qt.labs.controls 1.0

Rectangle {
    border.color: Theme.frameColor

    property date selectedDate
    property var locale
    property var eventModel

    signal addEventClicked

    Component {
        id: eventListHeader

        Row {
            id: eventDateRow
            width: parent.width
            height: eventDayLabel.height
            spacing: 10

            Label {
                id: eventDayLabel
                text: selectedDate.getDate()
                font.pointSize: 35
            }

            Column {
                height: eventDayLabel.height

                Label {
                    readonly property var options: { weekday: "long" }
                    text: Qt.locale().standaloneDayName(selectedDate.getDay(), Locale.LongFormat)
                    font.pointSize: 18
                }
                Label {
                    text: Qt.locale().standaloneMonthName(selectedDate.getMonth())
                          + selectedDate.toLocaleDateString(Qt.locale(), " yyyy")
                    font.pointSize: 12
                }
            }
        }
    }

    ListView {
        id: eventsListView
        spacing: 4
        clip: true
        header: eventListHeader
        anchors.fill: parent
        anchors.margins: 10
        model: eventModel

        delegate: Rectangle {
            width: eventsListView.width
            height: eventItemColumn.height
            anchors.horizontalCenter: parent.horizontalCenter

            Rectangle {
                width: parent.width
                height: 1
                color: "#eee"
            }

            Column {
                id: eventItemColumn
                x: 4
                y: 4
                width: parent.width - 8
                height: timeRow.height + descriptionLabel.height + 8

                Label {
                    id: descriptionLabel
                    width: parent.width
                    wrapMode: Text.Wrap
                    text: description
                }
                Row {
                    id: timeRow
                    width: parent.width
                    Label {
                        text: start.toLocaleTimeString(locale, Locale.ShortFormat)
                        color: "#aaa"
                    }
                    Label {
                        text: "-" + end.toLocaleTimeString(locale, Locale.ShortFormat)
                        visible: start.getTime() !== end.getTime() && start.getDate() === end.getDate()
                        color: "#aaa"
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                onPressAndHold: removeButton.opacity = 1
                onClicked: removeButton.opacity = 0
            }

            Button {
                id: removeButton
                opacity: 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: 150
                    }
                }

                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter

                onClicked: eventModel.removeEvent(index)

                background: Rectangle {
                    implicitWidth: 32
                    implicitHeight: 32

                    radius: width / 2
                    color: Qt.tint(!addButton.enabled ? addButton.Theme.disabledColor :
                                    addButton.activeFocus ? addButton.Theme.focusColor : "red",
                                    addButton.pressed ? addButton.Theme.pressColor : "transparent")
                }
            }

            // Don't want the white icon to change opacity.
            Rectangle {
                anchors.centerIn: removeButton
                width: 18
                height: 4
                radius: 1
            }
        }
    }

    Button {
        id: addButton

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12

        onClicked: addEventClicked()

        background: Rectangle {
            implicitWidth: 32
            implicitHeight: 32

            radius: width / 2
            color: Qt.tint(!addButton.enabled ? addButton.Theme.disabledColor :
                            addButton.activeFocus ? addButton.Theme.focusColor : addButton.Theme.accentColor,
                            addButton.pressed ? addButton.Theme.pressColor : "transparent")
        }

        Rectangle {
            anchors.centerIn: parent
            width: 4
            height: 18
            radius: 1
        }

        Rectangle {
            anchors.centerIn: parent
            width: 18
            height: 4
            radius: 1
        }
    }
}
