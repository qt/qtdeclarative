/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/
import QtQuick 2.0
import StockChart 1.0

Rectangle {
    id:container
    width: 700; height: 800
    StockModel {
        id:stockModel
        stockName: "NOK"
    }
    // Define a highlight with customised movement between items.
    Component {
        id: highlightBar
        Rectangle {
            width: 1000; height: 50
            color: "#FFFF88"
            y: view.currentItem.y;
            Behavior on y { SpringAnimation { spring: 2; damping: 0.1 } }
        }
    }
    // The delegate for each section header
    Component {
        id: sectionHeading
        Rectangle {
            width: container.width
            height: childrenRect.height
            color: "lightsteelblue"

            Text {
                text:stockModel.stockName + "(" + section  + ")" + "Open\t High\t Low\t Close\t Volume\t Adjusted"
                horizontalAlignment:Text.AlignHCenter
                font.bold: true
                font.underline:true
            }
        }
    }
    ListView {
        id:view
        width: parent.width
        anchors.fill: parent
        highlightFollowsCurrentItem: false
        focus: true
        keyNavigationWraps: true
        opacity: 0.8
        highlightRangeMode: ListView.StrictlyEnforceRange
        spacing:1
        model: stockModel
        highlight: Rectangle { color: "lightsteelblue" }
        section.property: "year"
        section.criteria: ViewSection.FullString
        section.delegate: sectionHeading
        delegate: Text { text: Qt.formatDate(date, "yyyy-MM-dd") + "\t " + Math.round(openPrice*100)/100 + "\t " + Math.round(highPrice*100)/100 + "\t " + Math.round(lowPrice*100)/100 + "\t " + Math.round(closePrice*100)/100 + "\t " + volume + "\t " + Math.round(adjustedPrice*100)/100 }


        property bool beginning:false

        Component.onCompleted: {
            view.positionViewAtBeginning();
        }

        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                if (view.beginning) {
                    view.positionViewAtBeginning();
                } else {
                    view.positionViewAtEnd();
                }
                view.beginning = !view.beginning;
            }
        }
    }
}

