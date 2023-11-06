import QtQuick
import Test

Item {
    id: root

    property int a: 0
    signal mySignal

    function test() {
        MyInheritedQmlObjectSingleton.value++
        root.a = MyInheritedQmlObjectSingleton.value
    }

    function disconnectSingleton() {
        root.mySignal.disconnect(MyInheritedQmlObjectSingleton, root.test)
    }

    Component.onCompleted: root.mySignal.connect(MyInheritedQmlObjectSingleton,
                                                 root.test)
}
