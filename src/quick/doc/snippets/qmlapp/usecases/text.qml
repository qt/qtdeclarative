// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Item {
    id: root
    width: 480
    height: 320

    Rectangle {
        color: "#272822"
        width: 480
        height: 320
    }

    Column {
        spacing: 20

        Text {
            text: 'I am the very model of a modern major general!'

            // color can be set on the entire element with this property
            color: "yellow"

        }

        Text {
            // For text to wrap, a width has to be explicitly provided
            width: root.width

            // This setting makes the text wrap at word boundaries when it goes
            // past the width of the Text object
            wrapMode: Text.WordWrap

            // You can use \ to escape quotation marks, or to add new lines (\n).
            //  Use \\ to get a \ in the string
            text: 'I am the very model of a modern major general. I\'ve information \
                  vegetable, animal and mineral. I know the kings of england and I \
                  quote the fights historical; from Marathon to Waterloo in order categorical.'

            // color can be set on the entire element with this property
            color: "white"

        }

        Text {
            text: 'I am the very model of a modern major general!'

            // color can be set on the entire element with this property
            color: "yellow"

            // font properties can be set effciently on the whole string at once
            font { family: 'Courier'; pixelSize: 20; italic: true; capitalization: Font.SmallCaps }

        }

        Text {
            // HTML like markup can also be used
            text: '<font color="white">I am the <b>very</b> model of a modern <i>major general</i>!</font>'

            // This could also be written font { pointSize: 14 }. Both syntaxes are valid.
            font.pointSize: 14

            // StyledText format supports fewer tags, but is more efficient than RichText
            textFormat: Text.StyledText
        }
    }
}
//![0]
