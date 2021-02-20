import QtQml

QtObject {
    id: root
    property QtObject testObject: QtObject {
        objectName: "frozen"
    }
    property bool caughtException: false
    property bool nameCorrect: false

    Component.onCompleted: () => {
        const frozen = Object.freeze(root.testObject)
        try {
            frozen.objectName = "thawed"
        } catch (e) {
            root.caughtException = true
        }
        root.nameCorrect = testObject.objectName === "frozen"
    }
}
