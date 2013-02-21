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
  width: 320
  height: 480

  // Create column with four rectangles, the fourth one is hidden
  Column {
    id: column

    //! [0]
    Rectangle {
      id: red
      color: "red"
      width: 100
      height: 100

      Text {
          anchors.left: parent.right
          anchors.leftMargin: 20
          anchors.verticalCenter: parent.verticalCenter
          text: "Index: " + parent.Positioner.index
          + (parent.Positioner.isFirstItem ? " (First)" : "")
          + (parent.Positioner.isLastItem ? " (Last)" : "")
      }

      // When mouse is clicked, display the values of the positioner
      MouseArea {
        anchors.fill: parent
        onClicked: column.showInfo(red.Positioner)
      }
    }
    //! [0]

    Rectangle {
      id: green
      color: "green"
      width: 100
      height: 100

      Text {
          anchors.left: parent.right
          anchors.leftMargin: 20
          anchors.verticalCenter: parent.verticalCenter
          text: "Index: " + parent.Positioner.index
          + (parent.Positioner.isFirstItem ? " (First)" : "")
          + (parent.Positioner.isLastItem ? " (Last)" : "")
      }

      // When mouse is clicked, display the values of the positioner
      MouseArea {
        anchors.fill: parent
        onClicked: column.showInfo(green.Positioner)
      }
    }

    Rectangle {
      id: blue
      color: "blue"
      width: 100
      height: 100

      Text {
          anchors.left: parent.right
          anchors.leftMargin: 20
          anchors.verticalCenter: parent.verticalCenter
          text: "Index: " + parent.Positioner.index
          + (parent.Positioner.isFirstItem ? " (First)" : "")
          + (parent.Positioner.isLastItem ? " (Last)" : "")
      }

      // When mouse is clicked, display the values of the positioner
      MouseArea {
        anchors.fill: parent
        onClicked: column.showInfo(blue.Positioner)
      }
    }

    // This rectangle is not visible, so it doesn't have a positioner value
    Rectangle {
      color: "black"
      width: 100
      height: 100
      visible: false
    }

    // Print the index of the child item in the positioner and convenience
    // properties showing if it's the first or last item.
    function showInfo(positioner) {
      console.log("Item Index = " + positioner.index)
      console.log("  isFirstItem = " + positioner.isFirstItem)
      console.log("  isLastItem = " + positioner.isLastItem)
    }
  }
}
