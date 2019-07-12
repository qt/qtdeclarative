import QtQuick 2.0

Item {
    id: root
    property int unqualified: 42

    Item {
        Item {
            x: unqualified // user defined property from root
        }

        QtObject {
            property int check: x // existing property from root
        }
    }

}
