import QtQuick 2.15

// tests that inline components get their own context

Item {
    id: root
    property string hello: "hello"
    component MyComponent: Item {
        property alias myHello: root.hello // not allowed: accessing stuff from outside component
    }
}
