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
import QtQuick.Calendar 2.0
import QtQuick.Controls 2.0

Item {
    id: dateTimePicker
    enabled: dateToShow.getFullYear() >= fromYear || dateToShow.getFullYear() <= toYear
    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    readonly property var days: [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

    readonly property int fromYear: 2000
    readonly property int toYear: 2020

    readonly property alias chosenDate: dateTimePicker.__date
    property var __date: new Date(
        fromYear + yearTumbler.currentIndex,
        monthTumbler.currentIndex,
        dayTumbler.currentIndex + 1,
        hoursTumbler.currentIndex + (amPmTumbler.currentIndex == 0 ? 0 : 12),
        minutesTumbler.currentIndex);

    property date dateToShow: new Date()
    onDateToShowChanged: {
        yearTumbler.currentIndex = dateToShow.getFullYear() - fromYear;
        monthTumbler.currentIndex = dateToShow.getMonth();
        dayTumbler.currentIndex = dateToShow.getDate() - 1;
    }

    FontMetrics {
        id: fontMetrics
    }

    Row {
        id: row
        spacing: 2

        Frame {
            padding: 0

            Row {
                Tumbler {
                    id: dayTumbler

                    delegate: TumblerDelegate {
                        text: modelData
                        font.pixelSize: fontMetrics.font.pixelSize * (AbstractTumbler.tumbler.activeFocus ? 2 : 1.25)
                    }

                    function updateModel() {
                        var previousIndex = dayTumbler.currentIndex;
                        var array = [];
                        var newDays = dateTimePicker.days[monthTumbler.currentIndex];
                        for (var i = 0; i < newDays; ++i) {
                            array.push(i + 1);
                        }
                        dayTumbler.model = array;
                        dayTumbler.currentIndex = Math.min(newDays - 1, previousIndex);
                    }

                    Component.onCompleted: updateModel()
                }
                Tumbler {
                    id: monthTumbler
                    model: ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
                    delegate: TumblerDelegate {
                        text: modelData
                        font.pixelSize: fontMetrics.font.pixelSize * (AbstractTumbler.tumbler.activeFocus ? 2 : 1.25)
                    }
                    onCurrentIndexChanged: dayTumbler.updateModel()
                }
                Tumbler {
                    id: yearTumbler
                    width: 80
                    model: {
                        var years = [];
                        for (var i = fromYear; i <= toYear; ++i) {
                            years.push(i);
                        }
                        return years;
                    }
                    delegate: TumblerDelegate {
                        text: modelData
                        font.pixelSize: fontMetrics.font.pixelSize * (AbstractTumbler.tumbler.activeFocus ? 2 : 1.25)
                    }
                }
            }
        }

        Frame {
            padding: 0

            Row {
                Tumbler {
                    id: hoursTumbler
                    model: 12
                    delegate: TumblerDelegate {
                        text: modelData.toString().length < 2 ? "0" + modelData : modelData
                        font.pixelSize: fontMetrics.font.pixelSize * (AbstractTumbler.tumbler.activeFocus ? 2 : 1.25)
                    }
                }

                Tumbler {
                    id: minutesTumbler
                    model: 60
                    delegate: TumblerDelegate {
                        text: modelData.toString().length < 2 ? "0" + modelData : modelData
                        font.pixelSize: fontMetrics.font.pixelSize * (AbstractTumbler.tumbler.activeFocus ? 2 : 1.25)
                    }
                }

                Tumbler {
                    id: amPmTumbler
                    model: ["AM", "PM"]
                    delegate: TumblerDelegate {
                        font.pixelSize: fontMetrics.font.pixelSize * (AbstractTumbler.tumbler.activeFocus ? 2 : 1.25)
                    }

                    contentItem: ListView {
                        anchors.fill: parent
                        model: amPmTumbler.model
                        delegate: amPmTumbler.delegate

                        snapMode: ListView.SnapToItem
                        highlightRangeMode: ListView.StrictlyEnforceRange
                        preferredHighlightBegin: height / 2 - (height / 3 / 2)
                        preferredHighlightEnd: height / 2  + (height / 3 / 2)
                        clip: true
                    }
                }
            }
        }
    }
}
