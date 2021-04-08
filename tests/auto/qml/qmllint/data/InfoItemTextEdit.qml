import QtQuick 2.11
import QtQuick.Window 2.3

Rectangle {
    property color textStrColor: "steelblue"
    property alias valueStr: input.text

    TextInput {
        id: input
    }

}
