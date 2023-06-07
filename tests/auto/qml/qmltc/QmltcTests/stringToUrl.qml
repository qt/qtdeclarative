import QtQuick 2.12

QtObject {
    readonly property FontLoader iconLoader: FontLoader {
        source: "qrc:/qt/qml/path/to/font.ttf"
    }
    property string myUrl: "qrc:/qt/qml/path/to/font2.ttf"
    readonly property FontLoader iconLoader2: FontLoader {
        source: myUrl
    }
}

