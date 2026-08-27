import QtQuick 2.0

Item {
    width: 320
    height: 480

    Text {
        anchors.centerIn: parent
        font.family: "Arial"
        font.pixelSize: 16
        textFormat: Text.RichText
        // The list item marker is sized from the <li> block char format, while the
        // marker's baseline follows the first line's ascent. Cover both directions:
        // a small item containing larger content, and a large item containing
        // smaller content (which gives a negative baseline offset).
        text: 'Unordered list:<br/>' +
              '<ul>' +
              '<li style="font-size:10px">small, <font size="+2">big</font> span</li>' +
              '<li style="font-size:28px">big, <span style="font-size:10px">small</span> span</li>' +
              '<li>plain item</li>' +
              '</ul>' +
              'Numbered list:<br/>' +
              '<ol>' +
              '<li style="font-size:10px">small, <font size="+2">big</font> span</li>' +
              '<li style="font-size:28px">big, <span style="font-size:10px">small</span> span</li>' +
              '<li>plain item</li>' +
              '</ol>' +
              'and some more text'
    }
}
