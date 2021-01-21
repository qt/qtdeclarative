import QtQuick 2.0
import test 1

Rectangle {
    width: 400
    height: 400

    WithBindableProperties {
        id: myItem
        objectName:  "myItem"
        a: 100 - myItem.b

        Binding on a {
            when: myItem.b > 50
            value: myItem.b
        }

        /*NumberAnimation on y {
            loops: Animation.Infinite
            to: 100
            duration: 1000
        }*/
    }
}
