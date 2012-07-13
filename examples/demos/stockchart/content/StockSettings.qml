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

Rectangle {
  id:root
  width:320
  height:480
  color:"#423A2F"
  property var startDate : startDatePicker.date;
  property var endDate : endDatePicker.date;

  property bool drawHighPrice:highButton.buttonEnabled
  property bool drawLowPrice:lowButton.buttonEnabled
  property bool drawOpenPrice:openButton.buttonEnabled
  property bool drawClosePrice:closeButton.buttonEnabled
  property bool drawVolume:volumeButton.buttonEnabled
  property bool drawKLine:klineButton.buttonEnabled

  property color highColor:Qt.rgba(1, 0, 0, 1)
  property color lowColor:Qt.rgba(0, 1, 0, 1)
  property color openColor:Qt.rgba(0, 0, 1, 1)
  property color closeColor:"#ecc088"
  property color volumeColor:Qt.rgba(0.3, 0.5, 0.7, 1)

  property string chartType:"year"
  Image {
    id:logo
    source:"images/logo.png"
    anchors.horizontalCenter : parent.horizontalCenter
    anchors.top:parent.top
    anchors.topMargin:15
  }

  Text {
    id:startDateText
    text:"START DATE:"
    color:"#76644A"
    font.pointSize:15
    anchors.left:parent.left
    anchors.leftMargin:20
    anchors.top:logo.bottom
    anchors.topMargin:20
  }

  DatePicker {
    id:startDatePicker
    anchors.left:parent.left
    anchors.leftMargin:30
    anchors.top:startDateText.bottom
    anchors.topMargin:15
    date : new Date(1995, 3, 25)
  }

  Text {
    id:endDateText
    text:"END DATE:"
    color:"#76644A"
    font.pointSize:15
    anchors.left:parent.left
    anchors.leftMargin:20
    anchors.top:startDatePicker.bottom
    anchors.topMargin:20
  }

  DatePicker {
    id:endDatePicker
    anchors.left:parent.left
    anchors.leftMargin:30
    anchors.top:endDateText.bottom
    anchors.topMargin:15
  }

  Text {
    id:drawOptionsText
    text:"DRAW OPTIONS:"
    color:"#76644A"
    font.pointSize:15
    anchors.left:parent.left
    anchors.leftMargin:20
    anchors.top:endDatePicker.bottom
    anchors.topMargin:20
  }
  Column {
      id:drawOptions
      anchors.top:drawOptionsText.bottom
      anchors.topMargin: 20
      anchors.left: parent.left
      anchors.leftMargin: 30
      spacing:2
       Row{
           spacing:10
          Text {
            text:"High   "
            color:"#76644A"
            font.pointSize:15
          }

          Button {
             id:highButton
             buttonEnabled:false
          }

          Text {
            text:"Low     "
            color:"#76644A"
            font.pointSize:15
          }

          Button {
             id:lowButton
             buttonEnabled:false
          }

          Text {
            text:"Open "
            color:"#76644A"
            font.pointSize:15
          }
          Button {
             id:openButton
             buttonEnabled:false
          }
      }
       Row{
           spacing:10
          Text {
            text:"Close "
            color:"#76644A"
            font.pointSize:15
          }
          Button {
             id:closeButton
             buttonEnabled:true
          }
          Text {
            text:"Volume"
            color:"#76644A"
            font.pointSize:15
          }
          Button {
             id:volumeButton
             buttonEnabled:true
          }
          Text {
            text:"K Line"
            color:"#76644A"
            font.pointSize:15
          }
          Button {
             id:klineButton
             buttonEnabled:false
          }
      }
  }


  Text {
    id:chartTypeText
    text:"CHART TYPE:"
    color:"#76644A"
    font.pointSize:15
    anchors.left:parent.left
    anchors.leftMargin:20
    anchors.top:drawOptions.bottom
    anchors.topMargin:20
  }
  Row {
      anchors.left: parent.left
      anchors.leftMargin: 20
      anchors.top : chartTypeText.bottom
      anchors.topMargin: 20
      spacing:10
      Rectangle {
          id:yearView
          width:70
          height:30
          radius:10
          color:"steelblue"
          Text {
              anchors.horizontalCenter: parent.horizontalCenter
              anchors.fill: parent
              font.pointSize: 15
              text:"YEAR"
          }
          MouseArea {
              anchors.fill: parent
              onClicked: {
                  if (root.chartType != "year") {
                      root.chartType = "year";
                      yearView.color = "steelblue"
                      monthView.color = "gray"
                      weekView.color = "gray"
                      allView.color = "gray"
                  }
              }
          }
      }
      Rectangle {
          id:monthView
          width:70
          radius:10
          height:30
          color:"gray"
          Text {
              anchors.fill: parent
              anchors.horizontalCenter: parent.horizontalCenter
              font.pointSize: 15
              color:"#ecc089"
              text:"MONTH"
          }
          MouseArea {
              anchors.fill: parent
              onClicked: {
                  if (root.chartType != "month") {
                      root.chartType = "month";
                      yearView.color = "gray"
                      monthView.color = "steelblue"
                      weekView.color = "gray"
                      allView.color = "gray"
                  }
              }
          }

      }
      Rectangle {
          id:weekView
          height:30
          width:70
          radius:10
          color:"gray"
          Text {
              anchors.fill: parent
              anchors.horizontalCenter: parent.horizontalCenter
              font.pointSize: 15
              color:"#ecc089"
              text:"WEEK"
          }
          MouseArea {
              anchors.fill: parent
              onClicked: {
                  if (root.chartType != "week") {
                      root.chartType = "week";
                      yearView.color = "gray"
                      monthView.color = "gray"
                      weekView.color = "steelblue"
                      allView.color = "gray"
                  }
              }
          }
      }
      Rectangle {
          id:allView
          width:70
          radius:10
          height:30
          color:"gray"
          Text {
              anchors.horizontalCenter: parent.horizontalCenter
              anchors.fill: parent
              font.pointSize: 15
              color:"#ecc089"
              text:"ALL"
          }
          MouseArea {
              anchors.fill: parent
              onClicked: {
                  if (root.chartType != "all") {
                      root.chartType = "all";
                      yearView.color = "gray"
                      monthView.color = "gray"
                      weekView.color = "gray"
                      allView.color = "steelblue"
                  }
              }
          }
      }
  }
}
