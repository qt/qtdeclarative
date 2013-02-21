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
    width: 320
    height: 480
    color: "#423A2F"

    property var stock: null
    property var stocklist: null
    property var settings: null
    signal listViewClicked
    signal settingsClicked

    function update() {
        chart.endDate = settings.endDate
        chart.update()
    }

    Rectangle {
        color: "#272822"
        anchors.fill: parent
        radius: 20

        Image {
            source: "images/icon-items.png"
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
            MouseArea {
                anchors.fill: parent
                onClicked: listViewClicked()
            }
        }
        Image {
            source: "images/icon-settings.png"
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
            MouseArea {
                anchors.fill: parent
                onClicked: settingsClicked()
            }
        }

        Text {
            id: desc
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.top: parent.top
            anchors.topMargin: 40
            color: "#564c3A"
            font.pointSize: 15
            text: root.stock.stockId + " - " + root.stock.stockName
        }

        Text {
            id: price
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.top: desc.bottom
            anchors.topMargin: 5
            color: "#ECC089"
            font.pointSize: 30
            text: root.stock.stockPrice
        }

        Text {
            id: priceChange
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.top: price.bottom
            anchors.topMargin: 5
            color: root.stock.stockPriceChanged < 0 ? "#A43D3D" : "#679B3A"
            font.pointSize: 25
            text: root.stock.stockPriceChanged + " (" + Math.abs(Math.round(root.stock.stockPriceChanged/(root.stock.stockPrice - root.stock.stockPriceChanged) * 100))/100  +"%)"
        }

        StockChart {
            id: chart
            anchors.bottom: parent.bottom
            anchors.top : priceChange.bottom
            anchors.topMargin: 30
            width: parent.width
            stockModel: root.stock
            settings: root.settings
        }
    }
}
