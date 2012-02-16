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
    id: main
    width: 320; height: 480
    focus: true

    property real offset: 0
    property real margin: 10

    Keys.onLeftPressed: myText.horizontalAlignment = Text.AlignLeft
    Keys.onUpPressed: myText.horizontalAlignment = Text.AlignHCenter
    Keys.onRightPressed: myText.horizontalAlignment = Text.AlignRight
    Keys.onDownPressed: myText.horizontalAlignment = Text.AlignJustify

    Text {
        id: myText
        anchors.fill: parent
        anchors.margins: 10
        wrapMode: Text.WordWrap
        font.family: "Times New Roman"
        font.pixelSize: 13
        textFormat: Text.StyledText
        horizontalAlignment: Text.AlignJustify

        text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer at ante dui sed eu egestas est facilis <a href=\"www.nokia.com\">www.nokia.com</a>.<br/>Curabitur ante est, pulvinar quis adipiscing a, iaculis id ipsum. Phasellus id neque id velit facilisis cursus ac sit amet nibh. Donec enim arcu, pharetra non semper nec, iaculis eget elit. Nunc blandit condimentum odio vel egestas.<br><ul type=\"bullet\"><li>Coffee<ol type=\"a\"><li>Espresso<li><b>Cappuccino</b><li><i>Flat White</i><li>Latte</ol><li>Juice<ol type=\"1\"><li>Orange</li><li>Apple</li><li>Pineapple</li><li>Tomato</li></ol></li></ul><p><font color=\"#434343\"><i>Proin consectetur <b>sapien</b> in ipsum lacinia sit amet mattis orci interdum. Quisque vitae accumsan lectus. Ut nisi turpis, sollicitudin ut dignissim id, fermentum ac est. Maecenas nec libero leo. Sed ac leo eget ipsum ultricies viverra sit amet eu orci. Praesent et tortor risus, viverra accumsan sapien. Sed faucibus eleifend lectus, sed euismod urna porta eu. Aenean ultricies lectus ut orci dictum quis convallis nisi ultrices. Nunc elit mi, iaculis a porttitor rutrum, venenatis malesuada nisi. Suspendisse turpis quam, euismod non imperdiet et, rutrum nec ligula. Lorem ipsum dolor sit amet, consectetur adipiscing elit."

        onLineLaidOut: {
            line.width = width / 2  - (2 * margin)
            if (line.number === 30) {
                main.offset = line.y
            }
            if (line.number >= 30) {
                line.x = width / 2 + margin
                line.y -= main.offset
            }
            if ((line.y + line.height) > rect.y && line.y < (rect.y + rect.height)) {
                if (line.number < 30)
                    line.width = Math.min((rect.x - line.x), line.width)
                else {
                    line.x = Math.max((rect.x + rect.width), width / 2 + margin)
                    line.width = Math.min((width - margin - line.x), line.width)
                }
            }
        }

        Item {
            id: rect
            x: 28; y: 20
            width: 60; height: 60

            Rectangle {
                anchors { fill: parent; leftMargin: 2; rightMargin: 2 }
                color: "lightsteelblue"; opacity: 0.3
            }

            MouseArea {
                anchors.fill: parent
                drag.target: rect
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: mouse.button == Qt.RightButton ? myText.font.pixelSize -= 1 : myText.font.pixelSize += 1
                onPositionChanged: myText.doLayout()
            }
        }
    }

}
