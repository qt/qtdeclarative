import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias control: ctrl

    Control {
        id: ctrl
        contentItem: Text {
            font.pointSize: 10.5
            elide: Text.ElideRight
            text: "This is some sample text"
        }
    }
}
