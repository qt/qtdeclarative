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
import QtQuick.Controls 2.0
import Qt.labs.calendar 1.0
import Qt.labs.templates 1.0 as T
import QtQuick.Layouts 1.0
import io.qt.examples.calendar 1.0

ApplicationWindow {
    id: window
    width: 640
    height: 400
    minimumWidth: 400
    minimumHeight: 300
    color: "#f4f4f4"
    visible: true
    title: "Qt Labs Calendar - Example"

    SqlEventModel {
        id: eventModel
        date: calendar.selectedDate
    }

    StackView {
        id: stackView
        anchors.fill: parent
        anchors.margins: 20

        initialItem: flow
    }

    Flow {
        id: flow
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
            currentIndex: model.indexOf(selectedDate.getFullYear(), selectedDate.getMonth() + 1)
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

                topPadding: title.height
                Column {
                    id: title
                    x: delegate.contentItem.x
                    width: delegate.contentItem.width
                    spacing: 6
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

                leftPadding: weekNumbers.width
                WeekNumberColumn {
                    id: weekNumbers
                    month: model.month
                    year: model.year
                    y: delegate.contentItem.y
                    height: delegate.contentItem.height
                }

                onClicked: calendar.selectedDate = date

                delegate: Text {
                    text: model.day
                    opacity: model.month === delegate.month ? 1 : 0
                    color: model.today ? Theme.accentColor : Theme.textColor
                    minimumPointSize: 8
                    fontSizeMode: Text.Fit
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    Rectangle {
                        z: -1
                        anchors.centerIn: parent
                        width: Math.min(parent.width * 0.6, parent.width * 0.6)
                        height: width
                        radius: width / 2
                        opacity: 0.5
                        color: pressed ? Theme.pressColor : "transparent";

                        SqlEventModel {
                            id: delegateEventModel
                        }

                        border.color: delegateEventModel.rowCount > 0 ? Theme.accentColor : "transparent"
                    }
                }
            }
            Rectangle {
                z: -1
                parent: calendar
                anchors.fill: parent
                border.color: Theme.frameColor
            }
        }

        EventView {
            width: (parent.width > parent.height ? (parent.width - parent.spacing) * 0.4 : parent.width)
            height: (parent.height > parent.width ? (parent.height - parent.spacing) * 0.4 : parent.height)
            selectedDate: calendar.selectedDate
            eventModel: eventModel
            locale: calendar.locale

            onAddEventClicked: stackView.push(createEventComponent)
        }
    }

    Component {
        id: createEventComponent

        ColumnLayout {
            spacing: 10
            visible: T.StackView.index === stackView.currentIndex

            DateTimePicker {
                id: dateTimePicker
                anchors.horizontalCenter: parent.horizontalCenter
                dateToShow: calendar.selectedDate
            }
            Frame {
                Layout.fillWidth: true

                TextArea {
                    id: descriptionField
                    placeholder.text: "Description"
                    anchors.fill: parent
                }
            }
            RowLayout {
                Layout.fillWidth: true

                Button {
                    text: "Cancel"
                    Layout.fillWidth: true
                    onClicked: stackView.pop()
                }
                Button {
                    text: "Create"
                    enabled: dateTimePicker.enabled
                    Layout.fillWidth: true
                    onClicked: {
                        eventModel.addEvent(descriptionField.text, dateTimePicker.chosenDate);
                        stackView.pop();
                    }
                }
            }
        }
    }
}
