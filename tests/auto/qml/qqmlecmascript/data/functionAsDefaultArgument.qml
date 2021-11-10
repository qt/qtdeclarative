import QtQml 2.15

QtObject {
    id: root

    function f(inner=function(){ root.objectName = "didRun" } ){ inner() }
    Component.onCompleted: f()
}
