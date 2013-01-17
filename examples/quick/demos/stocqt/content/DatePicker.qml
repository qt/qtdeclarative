/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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

import QtQuick 2.0

Rectangle {
    id: root
    color: "transparent"
    width: 300
    height: 40
    property var _monthNames: [ "JAN", "FEB", "MAR", "APR", "MAY", "JUN","JUL", "AUG", "SEP", "OCT", "NOV", "DEC" ];
    property var date: new Date()

    onDateChanged: {
        month.text = root._monthNames[root.date.getMonth()];
        day.text = date.getDate();
        year.text = date.getFullYear();
    }
    Row {
        spacing: 4
        anchors.fill: parent

        Rectangle {
            height: root.height
            width: root.width/3 - 20
            color: "#272822"
            border.color: "#76644A"
            border.width: 1
            radius: 2
            antialiasing: true

            TextInput {
                id: month
                anchors.centerIn: parent
                color: "#ecc089"
                font.pointSize: 25
                font.bold: true
                text: root._monthNames[root.date.getMonth()]
                onAccepted: {
                    for (var i = 0; i < 12; i++) {
                        if (text === root._monthNames[i]) {
                            root.date.setMonth(i);
                            root.date = root.date;
                            return;
                        }
                    }
                    root.date = root.date;
                }
            }
        }

        Rectangle {
            height: root.height
            width: root.width/3 - 20
            color: "#272822"
            border.color: "#76644A"
            border.width: 1
            radius: 2
            antialiasing: true

            TextInput {
                id: day
                anchors.centerIn: parent
                color: "#ecc089"
                font.pointSize: 25
                font.bold: true
                text: root.date.getDate()
                validator:IntValidator {bottom: 1; top: 31}
                onAccepted: { root.date.setDate(text); root.date = root.date;}
            }
        }

        Rectangle {
            height: root.height
            width: root.width/3 - 20
            color: "#272822"
            border.color: "#76644A"
            border.width: 1
            radius: 2
            antialiasing: true

            TextInput {
                id: year
                anchors.centerIn: parent
                color: "#ecc089"
                font.pointSize: 25
                font.bold: true
                text: root.date.getFullYear()
                validator: IntValidator {bottom: 1995; top: (new Date()).getFullYear()}
                onAccepted:{ root.date.setFullYear(text); root.date = root.date;}
            }
        }
    }
}
