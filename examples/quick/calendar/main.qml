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

import QtQuick 2.4
import QtQuick.Controls 2.0
import QtQuick.Calendar 2.0
import io.qt.examples.calendar 1.0

ApplicationWindow {
    id: window
    width: 640
    height: 400
    minimumWidth: 400
    minimumHeight: 300
    color: "#f4f4f4"
    visible: true
    title: "Qt Quick Controls - Calendar Example"

    SqlEventModel {
        id: eventModel
    }

    Flow {
        id: row
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10
        layoutDirection: Qt.RightToLeft

        ListView {
            id: calendar
            property date selectedDate: new Date()

            clip: true
            width: (parent.width > parent.height ? (parent.width - parent.spacing) * 0.6 : parent.width)
            height: (parent.height > parent.width ? (parent.height - parent.spacing) * 0.6 : parent.height)

            model: CalendarModel {
                id: model
                from: eventModel.min
                to: eventModel.max
            }

            focus: true
            currentIndex: -1
            snapMode: ListView.SnapOneItem
            highlightMoveDuration: 250
            highlightRangeMode: ListView.StrictlyEnforceRange
            orientation: parent.width > parent.height ? ListView.Vertical : ListView.Horizontal

            delegate: CalendarView {
                id: delegate

                width: calendar.width
                height: calendar.height

                month: model.month
                year: model.year

                padding.top: title.height
                Column {
                    id: title
                    x: delegate.contentItem.x
                    width: delegate.contentItem.width
                    spacing: window.style.spacing
                    Text {
                        width: parent.width
                        height: implicitHeight * 2
                        text: delegate.title
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 18
                    }
                    DayOfWeekRow {
                        width: parent.width
                    }
                }

                padding.left: weekNumbers.width
                WeekNumberColumn {
                    id: weekNumbers
                    month: model.month
                    year: model.year
                    y: delegate.contentItem.y
                    height: delegate.contentItem.height
                }

                onClicked: calendar.selectedDate = date

                delegate: CalendarDelegate {
                    text: model.day
                    width: delegate.contentItem.width / 7
                    height: delegate.contentItem.height / 6
                    opacity: model.month === delegate.month ? 1 : 0
                    color: model.today ? window.style.accentColor : window.style.textColor
                    Rectangle {
                        z: -1
                        anchors.centerIn: parent
                        width: Math.min(parent.width * 0.6, parent.width * 0.6)
                        height: width
                        radius: width / 2
                        opacity: 0.5
                        color: pressed ? window.style.pressColor : "transparent"
                        border.color: eventModel.eventsForDate(model.date).length > 0 ? window.style.accentColor : "transparent"
                    }
                }
            }
            Rectangle {
                z: -1
                parent: calendar
                anchors.fill: parent
                border.color: window.style.frameColor
            }
        }

        Component {
            id: eventListHeader

            Row {
                id: eventDateRow
                width: parent.width
                height: eventDayLabel.height
                spacing: 10

                Label {
                    id: eventDayLabel
                    text: calendar.selectedDate.getDate()
                    font.pointSize: 35
                }

                Column {
                    height: eventDayLabel.height

                    Label {
                        readonly property var options: { weekday: "long" }
                        text: Qt.locale().standaloneDayName(calendar.selectedDate.getDay(), Locale.LongFormat)
                        font.pointSize: 18
                    }
                    Label {
                        text: Qt.locale().standaloneMonthName(calendar.selectedDate.getMonth())
                              + calendar.selectedDate.toLocaleDateString(Qt.locale(), " yyyy")
                        font.pointSize: 12
                    }
                }
            }
        }

        Rectangle {
            width: (parent.width > parent.height ? (parent.width - parent.spacing) * 0.4 : parent.width)
            height: (parent.height > parent.width ? (parent.height - parent.spacing) * 0.4 : parent.height)
            border.color: window.style.frameColor

            ListView {
                id: eventsListView
                spacing: 4
                clip: true
                header: eventListHeader
                anchors.fill: parent
                anchors.margins: 10
                model: eventModel.eventsForDate(calendar.selectedDate)

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
                        height: timeRow.height + nameLabel.height + 8

                        Label {
                            id: nameLabel
                            width: parent.width
                            wrapMode: Text.Wrap
                            text: modelData.name
                        }
                        Row {
                            id: timeRow
                            width: parent.width
                            Label {
                                text: modelData.start.toLocaleTimeString(calendar.locale, Locale.ShortFormat)
                                color: "#aaa"
                            }
                            Label {
                                text: "-" + new Date(modelData.end).toLocaleTimeString(calendar.locale, Locale.ShortFormat)
                                visible: modelData.start.getTime() !== modelData.end.getTime() && modelData.start.getDate() === modelData.end.getDate()
                                color: "#aaa"
                            }
                        }
                    }
                }
            }
        }
    }
}
