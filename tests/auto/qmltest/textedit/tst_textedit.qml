/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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
import QtTest 1.0

Item {
    id: top

    TextEdit {
        id: emptyText
        height: 20
        width: 50
    }

    TextEdit {
        id: txtfamily
        text: "Hello world!"
        font.family: "Courier"
        height: 20
        width: 50
    }

    TextEdit {
        id: txtcolor
        text: "Hello world!"
        color: "red"
        height: 20
        width: 50
    }

    TextEdit {
        id: txtentry
        text: ""
        height: 20
        width: 50
    }

    TextEdit {
        id: txtentry2
        text: ""
        height: 20
        width: 50
    }

    TextEdit {
        id: txtfunctions
        text: "The quick brown fox jumped over the lazy dog"
        height: 20
        width: 50
    }

    TextEdit {
        id: txtlines
        property string styledtextvalue: "Line 1<br>Line 2<br>Line 3"
        text: "Line 1\nLine 2\nLine 3"
        textFormat: TextEdit.PlainText
    }

    TestCase {
        name: "TextEdit"
        when: windowShown

        function test_empty() {
            compare(emptyText.text, "")
        }

        function test_family() {
            compare(txtfamily.font.family, "Courier")
            txtfamily.font.family = "Helvetica";
            compare(txtfamily.font.family, "Helvetica")
        }

        function test_color() {
            compare(txtcolor.color, "#ff0000")
            txtcolor.color = "blue";
            compare(txtcolor.color, "#0000ff")
        }

        function test_textentry() {
            txtentry.focus = true;
            compare(txtentry.text, "")
            keyClick(Qt.Key_H)
            keyClick(Qt.Key_E)
            keyClick(Qt.Key_L)
            keyClick(Qt.Key_L)
            keyClick(Qt.Key_O)
            keyClick(Qt.Key_Space)
            keyClick(Qt.Key_W)
            keyClick(Qt.Key_O)
            keyClick(Qt.Key_R)
            keyClick(Qt.Key_L)
            keyClick(Qt.Key_D)
            compare(txtentry.text, "hello world")
        }

        function test_textentry_char() {
            txtentry2.focus = true;
            compare(txtentry2.text, "")
            keyClick("h")
            keyClick("e")
            keyClick("l")
            keyClick("l")
            keyClick("o")
            keyClick(" ")
            keyClick("W")
            keyClick("o")
            keyClick("r")
            keyClick("l")
            keyClick("d")
            compare(txtentry2.text, "hello World")
        }

        function test_functions() {
            compare(txtfunctions.getText(4,9), "quick")
            txtfunctions.select(4,9);
            compare(txtfunctions.selectedText, "quick")
            txtfunctions.deselect();
            compare(txtfunctions.selectedText, "")
            txtfunctions.select(4,9);
            txtfunctions.cut();
            compare(txtfunctions.text, "The  brown fox jumped over the lazy dog")
            txtfunctions.text = "Qt";
            txtfunctions.insert(txtfunctions.text.length, " ")
            compare(txtfunctions.text, "Qt ");
            txtfunctions.cursorPosition = txtfunctions.text.length;
            txtfunctions.paste();
            compare(txtfunctions.text, "Qt quick");
            txtfunctions.cursorPosition = txtfunctions.text.length;
            txtfunctions.selectWord();
            compare(txtfunctions.selectedText, "quick")
            txtfunctions.copy();
            txtfunctions.selectAll();
            compare(txtfunctions.selectedText, "Qt quick")
            txtfunctions.deselect();
            compare(txtfunctions.selectedText, "")
            txtfunctions.paste();
            compare(txtfunctions.text, "Qt quickquick");
        }

        function test_linecounts() {
            compare(txtlines.lineCount, 3)
            txtlines.text = txtlines.styledtextvalue;
            compare(txtlines.text, "Line 1<br>Line 2<br>Line 3")
            tryCompare(txtlines, 'lineCount', 1)
            txtlines.textFormat = TextEdit.RichText;
            tryCompare(txtlines, 'lineCount', 3)
        }
    }
}
