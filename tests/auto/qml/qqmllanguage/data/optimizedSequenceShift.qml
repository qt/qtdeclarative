import QtQml

QtObject {
    id: root

    property int changes: 0

    property list<int> numbers: [1, 2, 3, 4, 5]
    onNumbersChanged: ++changes

    property var one: numbers.shift.bind([1,2,3])()

    Component.onCompleted: root.numbers.shift()
}
