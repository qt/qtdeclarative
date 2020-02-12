import QtQuick 2.12
import io.qt.bugreports 2.0
Item {
    InterfaceConsumer2 {
        objectName: "a1"
        i: A2 {
            property int i: 42
        }
    }

    InterfaceConsumer2 {
        objectName: "a2"
        property A2 a: A2 {
            property int i: 43
        }
        i: a
    }

    InterfaceConsumer2 {
        objectName: "a3"
        property A2 a: A2 {
            id : aa
            property int i: 44
        }
        i: aa
    }
}
