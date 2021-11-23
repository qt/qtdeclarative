pragma Strict
import QtQuick

QtObject {
    id: self
    property color c: "#aabbcc"
    property color d: "#ccbbaa"
    property color e: "#" + "112233"
    Component.onCompleted: {
        c = "#dddddd";
        self.d = "#aaaaaa";
    }
}
