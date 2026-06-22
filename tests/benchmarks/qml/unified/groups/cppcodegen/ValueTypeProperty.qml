pragma Strict
import QtQml

QtObject {
    property rect r;
    property int inR: 12
    r.x: inR
    property int outR: r.x
}
