// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtTest 1.1

Item {
    id: top
    height: 30
    width: 60

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
        text: "The quick brown fox"
        height: 20
        width: 50
    }

    TextEdit {
        id: txtclipboard
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

        function test_select_insert() {
            compare(txtfunctions.getText(4,9), "quick")
            txtfunctions.select(4,9);
            compare(txtfunctions.selectedText, "quick")
            txtfunctions.insert(4, "very ")
            compare(txtfunctions.text, "The very quick brown fox")
            txtfunctions.deselect();
            compare(txtfunctions.selectedText, "")
            txtfunctions.text = "Qt";
            txtfunctions.insert(txtfunctions.text.length, " ")
            compare(txtfunctions.text, "Qt ");
            txtfunctions.insert(txtfunctions.text.length, "quick")
            compare(txtfunctions.text, "Qt quick");
            txtfunctions.cursorPosition = txtfunctions.text.length;
            txtfunctions.selectWord();
            compare(txtfunctions.selectedText, "quick")
            txtfunctions.selectAll();
            compare(txtfunctions.selectedText, "Qt quick")
            txtfunctions.deselect();
            compare(txtfunctions.selectedText, "")
        }

        function test_clipboard() {
            if (typeof(txtclipboard.copy) !== "function"
             || typeof(txtclipboard.paste) !== "function"
             || typeof(txtclipboard.cut) !== "function") {
                skip("Clipboard is not supported on this platform.")
            }
            txtclipboard.select(4,10);
            txtclipboard.cut();
            compare(txtclipboard.text, "The brown fox jumped over the lazy dog")
            txtclipboard.select(30,35)
            txtclipboard.paste();
            compare(txtclipboard.text, "The brown fox jumped over the quick dog")
            txtclipboard.text = "Qt ";
            txtclipboard.cursorPosition = txtclipboard.text.length;
            txtclipboard.paste();
            compare(txtclipboard.text, "Qt quick ");
            txtclipboard.cursorPosition = txtclipboard.text.length-1;
            txtclipboard.selectWord();
            compare(txtclipboard.selectedText, "quick")
            txtclipboard.copy();
            txtclipboard.cursorPosition = txtclipboard.text.length;
            txtclipboard.paste();
            compare(txtclipboard.text, "Qt quick quick");
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
