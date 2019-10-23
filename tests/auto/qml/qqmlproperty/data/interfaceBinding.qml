import QtQuick 2.12
import io.qt.bugreports 1.0
Item {
    InterfaceConsumer {
        objectName: "a1"
        i: A {
            property int i: 42
        }
    }

    InterfaceConsumer {
        objectName: "a2"
        property A a: A {
            property int i: 43
        }
        i: a
    }

    InterfaceConsumer {
        objectName: "a3"
        property A a: A {
            id : aa
            property int i: 44
        }
        i: aa
    }
}
