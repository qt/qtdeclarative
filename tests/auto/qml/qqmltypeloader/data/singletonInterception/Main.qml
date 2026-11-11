import QtQml

QtObject {
    property QtObject user1: User1 {}
    property QtObject user2: User2 {}
    property QtObject user3: User3 {}
    property QtObject user4: User4 {}
    property string names: Plain.objectName + " " + Selected.objectName
}
