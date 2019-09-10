import QtQuick 2.7

Item {
    id: _window
    property bool userFontStrikeout: true

    Component.onCompleted: {
            _box.font.strikeout = Qt.binding(function() { return _window.userFontStrikeout; });
    }

    Rectangle {
        id: _box
        width: 100
        height: 100
        property alias font: _text.font

        Text {
            id: _text
            anchors.fill: parent
            text: "Text"
        }
    }
}
