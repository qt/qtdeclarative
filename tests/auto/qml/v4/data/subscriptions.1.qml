import QtQuick 2.0

Rectangle {
    id: root
    width: 400
    height: 400

    property real targetHeight: menuItems.height + 1
    property real heightValue: if (1) menuItems.height      //this must be v8?
    property bool boolProp: menuItems.height > heightValue  //this must be v4?

    Column {
        id: menuItems
        Item { height: 200; width: 10 }
    }
}
