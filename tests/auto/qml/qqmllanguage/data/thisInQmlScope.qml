import QtQml 2.2
QtObject {
    property int x: 42
    property int y: 0
    function g(){
        y = this.x;
    }
    property var f: g
    Component.onCompleted: f()
}
