/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.LocalStorage
import "Database.js" as JS

Item {
    id: root
    width: Screen.width / 2
    height: Screen.height / 7

    required property ListView listView
    signal statusMessage(string msg)
    enabled: false

    function insertrec() {
        var rowid = parseInt(JS.dbInsert(dateInput.text, descInput.text, distInput.text), 10)
        if (rowid) {
            listView.model.setProperty(listView.currentIndex, "id", rowid)
            listView.forceLayout()
        }
        return rowid;
    }

    function editrec(Pdate, Pdesc, Pdistance, Prowid) {
        dateInput.text = Pdate
        descInput.text = Pdesc
        distInput.text = Pdistance
    }

    function initrec_new() {
        dateInput.clear()
        descInput.clear()
        distInput.clear()
        listView.model.insert(0, {
                                  date: "",
                                  trip_desc: "",
                                  distance: 0
                              })
        listView.currentIndex = 0
        dateInput.forceActiveFocus()
    }

    function initrec() {
        dateInput.clear()
        descInput.clear()
        distInput.clear()
    }

    function setlistview() {
        listView.model.setProperty(listView.currentIndex, "date",
                                   dateInput.text)
        listView.model.setProperty(listView.currentIndex, "trip_desc",
                                   descInput.text)
        listView.model.setProperty(listView.currentIndex, "distance",
                                   parseInt(distInput.text,10))
    }

    Rectangle {
        id: rootrect
        border.width: 10
        color: "#161616"

        ColumnLayout {
            id: mainLayout
            anchors.fill: parent

            Rectangle {
                id: gridBox
                Layout.fillWidth: true

                GridLayout {
                    id: gridLayout
                    rows: 3
                    flow: GridLayout.TopToBottom
                    anchors.fill: parent

                    Label {
                        text: qsTr("Date")
                        font.pixelSize: 22
                        rightPadding: 10
                    }

                    Label {
                        text: qsTr("Description")
                        font.pixelSize: 22
                        rightPadding: 10
                    }

                    Label {
                        text: qsTr("Distance")
                        font.pixelSize: 22
                    }

                    TextField {
                        id: dateInput
                        font.pixelSize: 22
                        activeFocusOnPress: true
                        activeFocusOnTab: true

                        ToolTip {
                            parent: dateInput
                            x: parent.width + 3
                            y: (parent.height - height) / 2
                            text: qsTr("Date format = 'YYYY-MM-DD'")
                            visible: parent.enabled && parent.hovered
                            delay: 1000
                        }

                        validator: RegularExpressionValidator {
                            regularExpression: /\d{4}[,.:/-]\d\d?[,.:/-]\d\d?/
                        }

                        onFocusChanged: ()=> {
                            if (!dateInput.focus && !acceptableInput && root.enabled)
                                root.statusMessage(qsTr("Please fill in the date"));
                        }

                        onEditingFinished: ()=> {
                            let regex = /(\d+)[,.:/-](\d+)[,.:/-](\d+)/
                            if (dateInput.text.match(regex))
                                dateInput.text = dateInput.text.replace(regex, '$1-$2-$3')
                        }
                    }

                    TextField {
                        id: descInput
                        font.pixelSize: 22
                        activeFocusOnPress: true
                        activeFocusOnTab: true
                        property string oldString
                        onFocusChanged: ()=> { if (focus) oldString = descInput.text; }
                        onEditingFinished: ()=> {
                            if (descInput.text.length < 8  && descInput.text != descInput.oldString && root.enabled)
                                root.statusMessage(qsTr("Enter a description of minimum 8 characters"))
                        }
                    }

                    TextField {
                        id: distInput
                        font.pixelSize: 22
                        activeFocusOnPress: true
                        activeFocusOnTab: true
                        validator: RegularExpressionValidator {
                            regularExpression: /\d{1,3}/
                        }
                        property string oldString
                        onFocusChanged: ()=> { if (focus) oldString = distInput.text; }
                        onEditingFinished: ()=> {
                            if (distInput.text == "" && distInput.text != distInput.oldString && root.enabled)
                                root.statusMessage(qsTr("Please fill in the distance"))
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: dateInput.forceActiveFocus()
    }
}
