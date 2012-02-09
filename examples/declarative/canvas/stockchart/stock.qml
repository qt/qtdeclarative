/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/
import QtQuick 2.0
import com.nokia.StockChartExample 1.0
import "../contents"

Rectangle {
    id:container
    width: 360; height: 600
    color: "#343434";
    Image { source: "contents/images/stripes.png"; fillMode: Image.Tile; anchors.fill: parent; opacity: 1 }


    TitleBar {
        id: titleBar
        width: parent.width
        anchors.top : container.top
        height: 40
        opacity: 0.9
    }

    StockModel {
        id:stockModel
        dataCycle: StockModel.Daily
        function dataCycleName() {
            if (dataCycle === StockModel.Weekly)
                return "Weekly";
            else if (dataCycle === StockModel.Monthly)
                return "Monthly";
            return "Daily";
        }

        onDataChanged: {
            if (view.viewType == "chart") {
                canvas.requestPaint();
            }
        }
        onDownloadProgress: {
            if (bytesReceived == bytesTotal && bytesTotal != -1) {
                progress.opacity = 0;
            } else {
                progress.opacity = 0.8;
                progress.text = "downloading " + stockModel.dataCycleName() + " data ..."+ Math.round(bytesReceived/1000) + " KB";
            }
        }

        property string description:"";
    }

    Stocks {id:stocks}

    Rectangle {
        id: header
        width: parent.width
        height: 20
        color: "steelblue"
        opacity: 0
        Row {
            spacing: 2
            Text {
                id:t
                font.pointSize:15
                horizontalAlignment:Text.AlignHCenter
                font.bold: true
                font.underline:true
            }
            Rectangle {
              height:20
              width:50
              Text {text:"Stock list"; font.pointSize:15; font.bold: true}
            }
        }
    }

    ListView {
        id:stockList
        width: parent.width
        anchors.bottom: container.bottom
        anchors.top : titleBar.bottom
        focus: true
        keyNavigationWraps: true
        spacing:1
        opacity: 1
        model: stocks

        Component.onCompleted: opacity = 0.9;
        onOpacityChanged: {
            titleBar.title = "Top 100 NASDAQ stocks"
        }


        delegate : Rectangle {
                height: 30
                width: view.width
                color: {
                    if (ListView.isCurrentItem)
                        return focus ? "lightyellow" : "pink";
                    return index % 2 == 0 ? "lightblue" : "lightsteelblue"
                }
                Text {
                       font.pointSize:20
                       text: index + ". " + stockId + " \t(" + name + ")";
                }
                MouseArea {
                    anchors.fill: parent;
                    onDoubleClicked: {
                        stockList.opacity = 0;
                        stockModel.stockName = stockId;
                        stockModel.description = "NASDAQ:" + stockId + " (" + name + ")";
                        view.opacity = 1;
                        view.viewType = "chart";
                        canvas.opacity = 0.7;
                    }
                    onClicked: stockList.currentIndex = index
                }//mousearea
        }//delegate
    }

    ListView {
        id:view
        width: container.width
        height: container.height - 50
        anchors.bottom: container.bottom
        focus: true
        keyNavigationWraps: true

        spacing:1
        opacity: 0
        model: stockModel
        highlightFollowsCurrentItem: false
        highlightRangeMode: ListView.StrictlyEnforceRange
        preferredHighlightBegin:50
        preferredHighlightEnd : height - 50
        highlight: listHighlight

        //header : Text {}
        delegate: listDelegate
        snapMode: ListView.SnapToItem

        property string viewType : "list"
        property int topIndex:indexAt(0,contentY);
        property int bottomIndex:indexAt(0, contentY+height);

        onCountChanged:  {

            titleBar.title = stockModel.description + " " + Qt.formatDate(stockModel.startDate, "yyyy-MM-dd") + " - " +
                    Qt.formatDate(stockModel.endDate, "yyyy-MM-dd") + " " + stockModel.dataCycleName() +
                             " records:" + view.count;

        }

        Component {
            id: listDelegate
            Rectangle {
                height: 20
                width: view.width
                border.color: "lightsteelblue"
                border.width: 1
                color: {
                    if (ListView.isCurrentItem)
                        return focus ? "lightyellow" : "pink";

                    return index % 2 == 0 ? "lightblue" : "lightsteelblue"
                }
                Text {
                       font.pointSize:13
                       text: index + ". " + Qt.formatDate(date, "yyyy-MM-dd")
                             + "\t " + Math.round(openPrice*100)/100
                             + "\t " + Math.round(highPrice*100)/100
                             + "\t " + Math.round(lowPrice*100)/100
                             + "\t " + Math.round(closePrice*100)/100
                             + "\t " + volume + "\t "
                             + Math.round(adjustedPrice*100)/100;
                }
                MouseArea {anchors.fill: parent; onClicked: view.currentIndex = index}
            }
        }

        Component {
            id: chartDelegate
            Rectangle {
                height: 20
                width: view.width/view.count * canvas.scaleX
                border.color: "lightsteelblue"
                border.width: 1
                color: {
                    if (ListView.isCurrentItem)
                        return focus ? "lightyellow" : "pink";

                    return index % 2 == 0 ? "lightblue" : "lightsteelblue"
                }

                Text {
                    anchors.bottom: parent.bottom
                    font.pointSize: {
                        if (parent.width <= 4)
                            return  1;
                        if (parent.width <= 50)
                            return parent.width/4;
                        return 15;
                    }
                    horizontalAlignment:Text.AlignHCenter
                    verticalAlignment:Text.AlignBottom
                    text:font.pointSize > 1 ? Qt.formatDate(date, "d/M/yy") : ""
                }
                MouseArea {anchors.fill: parent; onClicked: view.currentIndex = index}
            }
        }

        Component {
            id:chartHighlight
            Rectangle { radius: 5; width:40; height: 20; color: "lightsteelblue" }
        }

        Component {
            id:listHighlight
            Rectangle { radius: 5; width:container.width; height: 20; color: "lightsteelblue" }
        }




        onViewTypeChanged : {
            if (viewType == "list") {
                view.orientation = ListView.Vertical;
                view.delegate = listDelegate;
//                view.section.property = "year";
//                view.section.criteria = ViewSection.FullString;
//                view.section.delegate = sectionHeading;
                view.highlight = listHighlight;
                view.opacity = 1;
                canvas.opacity = 0;
               // comment.opacity = 0;

            } else if (viewType == "chart") {
                view.orientation = ListView.Horizontal;
                view.delegate = chartDelegate;
                //comment.opacity = 0.6;

                view.opacity = 1;
                view.height = 30

                canvas.opacity = 0.7;
                canvas.requestPaint();
            } else {
                viewType = "list";
            }
        }


        onCurrentIndexChanged: {
            //header.updateCurrent(stockModel.stockPriceAtIndex(view.currentIndex));
            if (viewType == "chart") {
                canvas.first = Math.round(view.currentIndex - view.currentIndex / canvas.scaleX);
                canvas.last = Math.round(view.currentIndex + (view.count - view.currentIndex) / canvas.scaleX);

                canvas.requestPaint();
            }
        }
        onContentYChanged: {    // keep "current" item visible
            topIndex = indexAt(0,contentY);
            bottomIndex = indexAt(0, contentY+height);

            if (topIndex != -1 && currentIndex <= topIndex)
                currentIndex = topIndex+1;
            else if (bottomIndex != -1 && currentIndex >= bottomIndex)
                currentIndex = bottomIndex-1;
            if (viewType == "chart")
                canvas.requestPaint();
        }

        onContentXChanged: {    // keep "current" item visible
            topIndex = indexAt(contentX,0);
            bottomIndex = indexAt(contentX+width, 0);

            if (topIndex != -1 && currentIndex <= topIndex)
                currentIndex = topIndex+1;
            else if (bottomIndex != -1 && currentIndex >= bottomIndex)
                currentIndex = bottomIndex-1;
            if (viewType == "chart")
                canvas.requestPaint();
        }

        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                if (view.viewType == "list")
                    view.viewType = "chart";
                else
                    view.viewType = "list";
            }
        }
    }



    Canvas {
        id:canvas
        anchors.top : titleBar.bottom
        anchors.bottom : view.top
        width:container.width;
        opacity:0
        renderTarget: Canvas.Image
        renderStrategy: Canvas.Immediate
        property bool running:false
        property int frames:first
        property int mouseX:0;
        property int mouseY:0;
        property int mousePressedX:0;
        property int mousePressedY:0;
        property int movedY:0
        property real scaleX:1.0;
        property real scaleY:1.0;
        property int first:0;
        property int last:view.count - 1;

        onOpacityChanged: {
            if (opacity > 0)
                requestPaint();
        }
        Text {
           id:comment
           x:100
           y:50
           font.pointSize: 20
           color:"white"
           opacity: 0.7
           focus:false
           text: stockModel.description
           function updateCurrent(price)
           {
               if (price !== undefined) {
                   text =stockModel.description + "\n"
                           + Qt.formatDate(price.date, "yyyy-MM-dd") + " OPEN:"
                           + Math.round(price.openPrice*100)/100 + " HIGH:"
                           + Math.round(price.highPrice*100)/100 + " LOW:"
                           + Math.round(price.lowPrice*100)/100 + " CLOSE:"
                           + Math.round(price.closePrice*100)/100 + " VOLUME:"
                           + price.volume;
               }
           }
        }

        Text {
            id:priceAxis
            x:25
            y:25
            font.pointSize: 15
            color:"yellow"
            opacity: 0.7
            focus: false
        }
        Text {
            id:volumeAxis
            x:canvas.width - 200
            y:25
            font.pointSize: 15
            color:"yellow"
            opacity: 0.7
        }

        Rectangle {
            id:progress
            x:canvas.width/2 - 100
            y:canvas.height/2
            width:childrenRect.width
            height: childrenRect.height
            opacity: 0
            color:"white"
            property string text;
            Text {
                text:parent.text
                font.pointSize: 20
            }
        }

        Button {
            id:runButton
            text:"Run this chart"
            y:0
            x:canvas.width/2 - 50
            opacity: 0.5
            onClicked:  {
                if (canvas.running) {
                    canvas.running = false;
                    canvas.frames = canvas.first;
                    canvas.requestPaint();
                    text = "Run this chart";
                    comment.text = stockModel.description;
                } else {
                    text = " Stop running ";
                    canvas.runChart();
                }
            }
        }
        Button {
            id:returnButton
            text:"Stocks"
            y:0
            anchors.left : runButton.right
            anchors.leftMargin : 20
            opacity: 0.5
            onClicked:  {
                stockList.opacity = 1;
                canvas.opacity = 0;
            }
        }
        PinchArea {
            anchors.fill: parent
            onPinchUpdated : {
                var current = pinch.center;
                var scale = pinch.scale;
                console.log("center:" + pinch.center + " scale:" + pinch.scale);
                //canvas.requestPaint();
            }
        }

        MouseArea {
            anchors.fill: parent

            onDoubleClicked: {
                if (stockModel.dataCycle == StockModel.Daily)
                    stockModel.dataCycle = StockModel.Weekly;
                else if (stockModel.dataCycle == StockModel.Weekly)
                    stockModel.dataCycle = StockModel.Monthly;
                else
                    stockModel.dataCycle = StockModel.Daily;
            }

            onPositionChanged: {
                if (mouse.modifiers & Qt.ControlModifier) {
                    if (canvas.mouseX == 0 && canvas.mouseY == 0) {
                        canvas.mouseX = mouse.x;
                        canvas.mouseY = mouse.y;
                    }
                } else{
                    var w = (view.width/view.count)*canvas.scaleX;

                    //canvas.movedY += Math.round((canvas.mousePressedY - mouse.y)/2);

                    var movedX = Math.round((canvas.mousePressedX - mouse.x)/w);
                    if (movedX != 0 || canvas.movedY != 0) {
                        if (canvas.first + movedX >= 0 && canvas.last + movedX < view.count) {
                            canvas.first += movedX;
                            canvas.last += movedX;
                        }
                        canvas.requestPaint();
                    }
                }
            }

            onPressed:  {
                canvas.mousePressedX = mouse.x;
                canvas.mousePressedY = mouse.y;
            }

            onReleased : {
                if (mouse.modifiers & Qt.ControlModifier) {
                    var sx = mouse.x - canvas.mouseX;
                    var sy = canvas.mouseY - mouse.y;

                    if (Math.abs(sx) < 50) sx = 0;
                    if (Math.abs(sy) < 50) sy = 0;

                    if (sx > 0)
                        canvas.scaleX *= sx/100 +1;
                    else
                        canvas.scaleX *= 1/(-sx/100 + 1);

                    if (sy > 0)
                        canvas.scaleY *= sy/100 +1;
                    else
                        canvas.scaleY *= 1/(-sy/100 + 1);

                    if (canvas.scaleX < 1)
                        canvas.scaleX = 1;

                    //console.log("scaleX:" + canvas.scaleX + ", scaleY:" + canvas.scaleY);

                    canvas.first = Math.round(view.currentIndex - view.currentIndex / canvas.scaleX);
                    canvas.last = Math.round(view.currentIndex + (view.count - view.currentIndex) / canvas.scaleX);

                    canvas.mouseX = 0;
                    canvas.mouseY = 0;
                    canvas.mousePressedX = 0;
                    canvas.mousePressedY = 0;
                    canvas.requestPaint();
                }
            }
        }

        function runChart() {
           canvas.running = true;
            requestPaint();
        }

        function showPriceAt(x) {
            var w = (view.width/view.count)*canvas.scaleX;
            //header.updateCurrent(stockModel.stockPriceAtIndex(canvas.first + Math.round(x/w)));
            //console.log("x:" + x + " w:" + w + " index:" + (canvas.first + Math.round(x/w)));
        }

        function drawPrice(ctx, from, to, color, price, points, highest)
        {
            ctx.globalAlpha = 0.7;
            ctx.strokeStyle = color;
            ctx.lineWidth = 1;
            ctx.beginPath();

            //price x axis
            priceAxis.text = "price:" + Math.round(highest);
            ctx.font = "bold 12px sans-serif";

            ctx.strokeText("price", 25, 25);
            for (var j = 1; j < 30; j++) {
                var val = (highest * j) / 30;
                val = canvas.height * (1 - val/highest);
                ctx.beginPath();

                ctx.moveTo(10, val);
                if (j % 5)
                    ctx.lineTo(15, val);
                else
                    ctx.lineTo(20, val);
                ctx.stroke();
            }

            ctx.beginPath();
            ctx.moveTo(10, 0);
            ctx.lineTo(10, canvas.height);
            ctx.stroke();


            var w = canvas.width/points.length;
            var end = canvas.running? canvas.frames - canvas.first :points.length;
            for (var i = 0; i < end; i++) {
                var x = points[i].x;
                var y = points[i][price];
                y += canvas.movedY;

                y = canvas.height * (1 - y/highest);
                if (i == 0) {
                    ctx.moveTo(x+w/2, y);
                } else {
                    ctx.lineTo(x+w/2, y);
                }
            }
            ctx.stroke();
        }

        function drawKLine(ctx, from, to, points, highest)
        {
            ctx.globalAlpha = 0.4;
            ctx.lineWidth = 2;
            var end = canvas.running? canvas.frames - canvas.first :points.length;
            for (var i = 0; i < end; i++) {
                var x = points[i].x;
                var open = canvas.height * (1 - points[i].open/highest) - canvas.movedY;
                var close = canvas.height * (1 - points[i].close/highest) - canvas.movedY;
                var high = canvas.height * (1 - points[i].high/highest) - canvas.movedY;
                var low = canvas.height * (1 - points[i].low/highest) - canvas.movedY;

                var top, bottom;
                if (close <= open) {
                    ctx.fillStyle = Qt.rgba(1, 0, 0, 1);
                    ctx.strokeStyle = Qt.rgba(1, 0, 0, 1);
                    top = close;
                    bottom = open;
                } else {
                    ctx.fillStyle = Qt.rgba(0, 1, 0, 1);
                    ctx.strokeStyle = Qt.rgba(0, 1, 0, 1);
                    top = open;
                    bottom = close;
                }

                var w1, w2;
                w1 = canvas.width/points.length;
                w2 = w1 > 10 ? w1/2 : w1;

                ctx.fillRect(x + (w1 - w2)/2, top, w2, bottom - top);
                ctx.beginPath();
                ctx.moveTo(x+w1/2, high);
                ctx.lineTo(x+w1/2, low);
                ctx.stroke();
            }
            ctx.globalAlpha = 1;

        }

        function drawVolume(ctx, from, to, color, price, points, highest)
        {
            ctx.fillStyle = color;
            ctx.globalAlpha = 0.6;
            ctx.strokeStyle = Qt.rgba(0.8, 0.8, 0.8, 1);
            ctx.lineWidth = 1;

            //volume x axis
            volumeAxis.text = "volume:" + Math.round(highest/(1000*100))  + "M";
            for (var j = 1; j < 30; j++) {
                var val = (highest * j) / 30;
                val = canvas.height * (1 - val/highest);
                ctx.beginPath();
                if (j % 5)
                    ctx.moveTo(canvas.width - 15, val);
                else
                    ctx.moveTo(canvas.width - 20, val);
                ctx.lineTo(canvas.width - 10, val);
                ctx.stroke();
            }

            ctx.beginPath();
            ctx.moveTo(canvas.width - 10, 0);
            ctx.lineTo(canvas.width - 10, canvas.height);
            ctx.stroke();

            var end = canvas.running? canvas.frames - canvas.first :points.length;
            for (var i = 0; i < end; i++) {
                var x = points[i].x;
                var y = points[i][price];
                y = canvas.height * (1 - y/highest);
                ctx.fillRect(x, y, canvas.width/points.length, canvas.height - y);
            }
        }
/*
        onPainted : {
            if (canvas.running) {
                if (frames >= last) {
                    canvas.running = false;
                    canvas.frames = first;
                    runButton.text = "Run this chart";
                    comment.text = stockModel.description;
                    requestPaint();
                } else {
                    frames += Math.round(view.count / 100);
                    if (frames > last) frames = last;
                    var price = stockModel.stockPriceAtIndex(frames);
                    if (price) {
                        comment.updateCurrent(price);
                    }

                    requestPaint();
                }
            }
        }
*/
        onPaint: {
            if (view.currentIndex <= 0)
                first = 0;
            if (last >= view.count)
                last = view.count - 1;

            //console.log("painting...  first:" + first + ", last:" + last + " current:" + view.currentIndex);
            var ctx = canvas.getContext("2d");
            ctx.reset();

            ctx.globalCompositeOperation = "source-over";
            ctx.lineWidth = 1;
            ctx.lineJoin = "round";
            ctx.fillStyle = "rgba(0,0,0,0)";

            ctx.fillRect(0, 0, canvas.width, canvas.height);



            var highestPrice = 500/canvas.scaleY;
            var highestValume = 600 * 1000 * 1000/canvas.scaleY;
            var points = [];
            for (var i = 0; i <= last - first; i++) {
                var price = stockModel.stockPriceAtIndex(i + first);
                points.push({
                             x: i*canvas.width/(last-first+1),
                             open: price.openPrice,
                             close: price.closePrice,
                             high:price.highPrice,
                             low:price.lowPrice,
                             volume:price.volume
                            });
            }

            drawPrice(ctx, first, last, Qt.rgba(1, 0, 0, 1),"high", points, highestPrice);
            drawPrice(ctx, first, last, Qt.rgba(0, 1, 0, 1),"low", points, highestPrice);
            drawPrice(ctx, first, last, Qt.rgba(0, 0, 1, 1),"open", points, highestPrice);
            drawPrice(ctx, first, last, Qt.rgba(0.5, 0.5, 0.5, 1),"close", points, highestPrice);
            drawVolume(ctx, first, last, Qt.rgba(0.3, 0.5, 0.7, 1),"volume", points, highestValume);
            drawKLine(ctx, first, last, points, highestPrice);
        }
    }
}