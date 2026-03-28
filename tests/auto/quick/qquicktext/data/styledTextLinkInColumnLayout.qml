import QtQuick
import QtQuick.Layouts

ColumnLayout {
    width: 640
    height: 480

    Text {
        objectName: "styledTextLink"
        text: 'this text has a <a href="http://qt-project.org/test">link</a> in it'
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        onLinkHovered: function(link) { hoveredLinkText.text = link }
    }

    Text {
        objectName: "hoveredLink"
        id: hoveredLinkText
    }
}
