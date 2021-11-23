pragma Strict
import QtQml

QtObject {
    id: self
    property int ppp: 3
    property int ppp2: ppp * 3
    property int ppp3: self.ppp * 4
}
