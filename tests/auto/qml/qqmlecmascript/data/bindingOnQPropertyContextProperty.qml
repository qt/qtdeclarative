import Qt.test 1.0
import QtQuick 2

Item {
    id: root
    property ClassWithQProperty2 testee: null
    Repeater {
        model: 2
        Item {
            ClassWithQProperty2 {
                id: myself
                value: index+1
                Component.onCompleted: {if (index == 1) {root.testee = myself}}
            }
        }
    }
}
