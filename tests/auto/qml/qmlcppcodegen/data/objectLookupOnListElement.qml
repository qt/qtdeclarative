pragma Strict
import QtQuick

Item {
    id: stack

    property int current: 0

    onCurrentChanged: setZOrders()
    Component.onCompleted: setZOrders()

    function setZOrders() {
        for (var i = 0; i < Math.max(stack.children.length, 3); ++i) {
            stack.children[i].z = (i == current ? 1 : 0)
            stack.children[i].enabled = (i == current)
        }
    }

    function zOrders() : list<int> {
        return [
            stack.children[0].z,
            stack.children[1].z,
            stack.children[2].z
        ]
    }

    function clearChildren() {
        children.length = 0;
    }

    Item {}
    Item {}
    Item {}
}
