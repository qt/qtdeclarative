pragma Strict
import QtQml

QtObject {
    function a() {}
    property var aa: a()
    function b() : void {}
    property var bb: b()
}
