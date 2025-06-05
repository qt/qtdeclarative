import QtQuick

Item {
    id: root

    Item {
        id: inner

        Tumbler {
            id: year

            delegate: Rectangle {
                required property var modelData
            }
        }

        Tumbler {
            id: month

            delegate: Rectangle {
                required property var modelData
            }
        }
    }
}
