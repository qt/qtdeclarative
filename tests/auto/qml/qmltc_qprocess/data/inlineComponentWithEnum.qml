import QtQuick 2.15

Item {
    component MyInlineComponent: Item {
        enum MyEnum { This, Is, Not, Allowed }
    }
}
