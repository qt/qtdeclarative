pragma Strict
import QtQml

QtObject {
    id: self
    property url c: "http://aabbcc.com"
    property url d: "http://ccbbaa.com"
    property url e: "http:" + "//a112233.de"
    Component.onCompleted: {
        c = "http://dddddd.com";
        self.d = "http://aaaaaa.com";
    }
}
