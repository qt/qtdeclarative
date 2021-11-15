import QtQuick 2.0

Item {
    width: 320
    height: 480

    Text {
        anchors.fill: parent
        anchors.margins: 10
        font.pixelSize: 18
        textFormat: Text.RichText
        text: '<hr>' +
              '<hr/>' +
              '<hr width="50%"/>' +
              '<hr width="70%" style="background-color:#c0fefe; border-width: 1;"/>' +
              '<hr width="80" style="background-color:darkblue;"/>' +
              '<hr style="background-color:maroon; height: 1px; border-width: 0;"/>'
    }
}
