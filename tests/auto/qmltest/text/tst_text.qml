// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtTest 1.1

Item {
    id: top

    Text {
        id: emptyText
    }

    Text {
        id: txtfamily
        text: "Hello world!"
        font.family: "Courier"
    }

    Text {
        id: txtcolor
        text: "Hello world!"
        color: "red"
    }

    Text {
        id: txtelide
        text: "Hello world!"
        elide: Text.ElideRight
    }

    Text {
        property string first: "Hello world!"
        property string second: "Hello\nworld!"
        property string third: "Hello\nworld\n!"

        id: txtlinecount
        text: first
        width: 120
        wrapMode: Text.WrapAnywhere
        font.pixelSize: 18
    }

    Text {
        id: txtlines
        property string styledtextvalue: "Line 1<br>Line 2<br>Line 3"
        text: "Line 1\nLine 2\nLine 3"
        textFormat: Text.PlainText
    }

    TestCase {
        name: "Text"

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

        function test_elide() {
            compare(txtelide.elide, Text.ElideRight)
            txtelide.elide = Text.ElideLeft;
            compare(txtelide.elide, Text.ElideLeft)
            txtelide.elide = Text.ElideMiddle;
            compare(txtelide.elide, Text.ElideMiddle)
        }

        function test_linecount() {
            compare(txtlinecount.lineCount, 1)
            txtlinecount.text = txtlinecount.second;
            compare(txtlinecount.lineCount, 2)
            txtlinecount.text = txtlinecount.third;
            compare(txtlinecount.lineCount, 3)
            console.log(txtlinecount.width)
            txtlinecount.text = txtlinecount.first;
            compare(txtlinecount.lineCount, 1)
            txtlinecount.width = 44;
            compare(txtlinecount.lineCount, 3)
        }
        function test_linecounts() {
            compare(txtlines.lineCount, 3)
            txtlines.text = txtlines.styledtextvalue;
            compare(txtlines.text, "Line 1<br>Line 2<br>Line 3")
            tryCompare(txtlines, 'lineCount', 1)
            txtlines.textFormat = Text.StyledText;
            tryCompare(txtlines, 'lineCount', 3)
            txtlines.textFormat = Text.RichText;
            tryCompare(txtlines, 'lineCount', 3)
        }

    }
}
