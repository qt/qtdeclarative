import QtQml

QtObject {
    property list<rect> rectList: [ Qt.rect(1,2,3,4), Qt.rect(5,6,7,8), Qt.rect(9,10,11,12) ]
    property list<string> stringList: ["aaa", "bbb", "ccc"]
    property list<int> intList: [5, 6, 7, 8]
    property string charList: "abcde"

    property var rectInBounds: rectList[0]
    property var rectOutOfBounds: rectList[32]
    property var stringInBounds: stringList[1]
    property var stringOutOfBounds: stringList[33]
    property var intInBounds: intList[2]
    property var intOutOfBounds: intList[34]
    property var charInBounds: charList[3]
    property var charOutOfBounds: charList[35]
}
