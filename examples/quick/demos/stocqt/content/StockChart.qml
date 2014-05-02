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
    id: chart
    width: 320
    height: 200
    color: "transparent"

    property var _months: [ "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" ]
    property var stockModel: null
    property var startDate: new Date()
    property var endDate: new Date()
    property string activeChart: "year"
    property var settings

    function update() {
        endDate = new Date();
        if (chart.activeChart === "year")
            chart.startDate = new Date(chart.endDate.getFullYear() - 1, chart.endDate.getMonth(), chart.endDate.getDate());
        else if (chart.activeChart === "month")
            chart.startDate = new Date(chart.endDate.getFullYear() , chart.endDate.getMonth() -1, chart.endDate.getDate());
        else if (chart.activeChart === "week")
            chart.startDate = new Date(chart.endDate.getFullYear() , chart.endDate.getMonth(), chart.endDate.getDate() - 7);
        else
            chart.startDate = new Date(2005, 3, 25);

        canvas.requestPaint();
    }

    Row {
        id: activeChartRow
        anchors.left: chart.left
        anchors.right: chart.right
        anchors.top: chart.top
        anchors.topMargin: 4
        spacing: 52
        onWidthChanged: {
            var buttonsLen = maxButton.width + yearButton.width + monthButton.width + weekButton.width;
            var space = (width - buttonsLen) / 3;
            spacing = Math.max(space, 10);
        }

        Button {
            id: maxButton
            text: "Max"
            buttonEnabled: chart.activeChart === "max"
            onClicked: {
                chart.activeChart = "max";
                chart.update();
            }
        }
        Button {
            id: yearButton
            text: "Year"
            buttonEnabled: chart.activeChart === "year"
            onClicked: {
                chart.activeChart = "year";
                chart.update();
            }
        }
        Button {
            id: monthButton
            text: "Month"
            buttonEnabled: chart.activeChart === "month"
            onClicked: {
                chart.activeChart = "month";
                chart.update();
            }
        }
        Button {
            id: weekButton
            text: "Week"
            buttonEnabled: chart.activeChart === "week"
            onClicked: {
                chart.activeChart = "week";
                chart.update();
            }
        }
    }

    Text {
        id: fromDate
        color: "#000000"
        width: 50
        font.family: "Open Sans"
        font.pointSize: 10
        wrapMode: Text.WordWrap
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.bottom: parent.bottom
        text: _months[startDate.getMonth()] + "\n" + startDate.getFullYear()
    }

    Text {
        id: toDate
        color: "#000000"
        font.pointSize: 10
        width: 50
        wrapMode: Text.WordWrap
        anchors.right: parent.right
        anchors.leftMargin: 20
        anchors.bottom: parent.bottom
        text: _months[endDate.getMonth()] + "\n" + endDate.getFullYear()
    }

    Canvas {
        id: canvas
        anchors.top: activeChartRow.bottom
        anchors.left: chart.left
        anchors.right: chart.right
        anchors.bottom: fromDate.top
        antialiasing: true

        property int pixelSkip: 1

        property real xGridOffset: width / 13
        property real xGridStep: width / 4
        property real yGridOffset: height / 26
        property real yGridStep: height / 12

        function drawBackground(ctx) {
            ctx.save();
            ctx.fillStyle = "#ffffff";
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            ctx.strokeStyle = "#d7d7d7";
            ctx.beginPath();
            // Horizontal grid lines
            for (var i = 0; i < 12; i++) {
                ctx.moveTo(0, canvas.yGridOffset + i * canvas.yGridStep);
                ctx.lineTo(canvas.width, canvas.yGridOffset + i * canvas.yGridStep);
            }

            // Vertical grid lines
            var height = 35 * canvas.height / 36;
            var yOffset = canvas.height - height;
            for (i = 0; i < 4; i++) {
                ctx.moveTo(canvas.xGridOffset + i * canvas.xGridStep, yOffset);
                ctx.lineTo(canvas.xGridOffset + i * canvas.xGridStep, height);
            }
            ctx.stroke();

            // Right ticks
            ctx.strokeStyle = "#666666";
            ctx.beginPath();
            var xStart = 35 * canvas.width / 36;
            ctx.moveTo(xStart, 0);
            ctx.lineTo(xStart, canvas.height);
            for (i = 0; i < 12; i++) {
                ctx.moveTo(xStart, canvas.yGridOffset + i * canvas.yGridStep);
                ctx.lineTo(canvas.width, canvas.yGridOffset + i * canvas.yGridStep);
            }
            ctx.moveTo(0, canvas.yGridOffset + 9 * canvas.yGridStep);
            ctx.lineTo(canvas.width, canvas.yGridOffset + 9 * canvas.yGridStep);
            ctx.closePath();
            ctx.stroke();

            ctx.restore();
        }

        function drawPrice(ctx, from, to, color, price, points, highest)
        {
            ctx.save();
            ctx.globalAlpha = 0.7;
            ctx.strokeStyle = color;
            ctx.lineWidth = 3;
            ctx.beginPath();

            var end = points.length;
            var xOffset = canvas.width / 36

            for (var i = 0; i < end; i += pixelSkip) {
                var x = 34 * points[i].x / 36 + xOffset;
                var y = points[i][price];

                y = canvas.height * y/highest;
                y = canvas.height - y;
                y = 9 * y / 12 + yGridOffset; // Scaling to graph area
                if (i == 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }
            }
            ctx.stroke();
            ctx.restore();
        }

        function drawVolume(ctx, from, to, color, price, points, highest)
        {
            ctx.save();
            ctx.fillStyle = color;
            ctx.globalAlpha = 0.6;
            ctx.strokeStyle = Qt.rgba(0.8, 0.8, 0.8, 1);
            ctx.lineWidth = 1;

            var end = points.length;
            for (var i = 0; i < end; i+=pixelSkip) {
                var x = points[i].x;
                var y = points[i][price];
                y = canvas.height * (1 - y/highest);
                y = 3 * y / 13;
                ctx.fillRect(x, canvas.height - y, canvas.width/points.length, y);
            }
            ctx.restore();
        }

        onPaint: {
            var ctx = canvas.getContext("2d");

            ctx.globalCompositeOperation = "source-over";
            ctx.lineWidth = 1;

            drawBackground(ctx);

            if (!stockModel.ready) {
                return;
            }

            var last = stockModel.indexOf(chart.startDate);
            var first = 0;

            var highestPrice = stockModel.highestPrice;
            var highestVolume = stockModel.highestVolume;
            var points = [];
            var step = canvas.width / (last + 0);
            for (var i = last, j = 0; i >= 0 ; i -= pixelSkip, j += pixelSkip) {
                var price = stockModel.get(i);
                points.push({
                                x: j * step,
                                open: price.open,
                                close: price.close,
                                high: price.high,
                                low: price.low,
                                volume: price.volume
                            });
            }

            if (settings.drawHighPrice)
                drawPrice(ctx, first, last, settings.highColor, "high", points, highestPrice);
            if (settings.drawLowPrice)
                drawPrice(ctx, first, last, settings.lowColor, "low", points, highestPrice);
            if (settings.drawOpenPrice)
                drawPrice(ctx, first, last,settings.openColor, "open", points, highestPrice);
            if (settings.drawClosePrice)
                drawPrice(ctx, first, last, settings.closeColor, "close", points, highestPrice);
            drawVolume(ctx, first, last, settings.volumeColor, "volume", points, highestVolume);
        }
    }
}
