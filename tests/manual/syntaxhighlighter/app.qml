// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import Highlighter

TextEdit {
    id: textEdit
    width: 420; height: 200
    text: '<p>Try the different <span style="text-decoration: underline overline; text-decoration-color: green;">highlight</span> styles.' +
          "The keyword is 'char'.</p><code>char * test;</code>" + '<ol>
<li style="color:#f32727">Red bullet</li>
<li style="color:#f327ca">Pink bullet</li>
<li>black bullet</li>
<li style="color:#f32727">Red bullet</li></ol><p>
Some non-red text<p>'
    textFormat: TextEdit.RichText
    leftPadding: 6; topPadding: 6

    DocumentHighlighter {
        id: highlighter
        document: textEdit.textDocument
        style: sb.value
    }

    SpinBox {
        id: sb
        anchors.bottom: parent.bottom
        anchors.margins: 6
        x: 6
        to: items.length - 1
        value: 0
        width: 200

        property var items: ["Bold", "Cyanderlined", "Misspelled", "Gaudy", "Orangeout"]

        validator: RegularExpressionValidator {
            regularExpression: new RegExp("(Bold|Cyanderlined|Misspelled|Gaudy|Orangeout)", "i")
        }

        textFromValue: function(value) {
            return items[value];
        }

        valueFromText: function(text) {
            for (var i = 0; i < items.length; ++i) {
                if (items[i].toLowerCase().indexOf(text.toLowerCase()) === 0)
                    return i
            }
            return sb.value
        }
    }
}
