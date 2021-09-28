import QtQuick

Item {
    property list<Item> array: [
        Item {
        },
        Item {
            nr: 2
        }
    ]
    property var array2: [1, 2, 3]
    property var arrayNoMerge
    default property list<Item> nonMergeable
    property list<Item> singleObjList

    arrayNoMerge: [
        Item {
        },
        Item {
            nr: 2
        }
    ]
    nonMergeable: [
        Item {
        },
        Item {
            nr: "II"
        }
    ]
    singleObjList: Item {
        el: "alone"
    }
}
