pragma Strict
import QtQml

QtObject {
    property list<string> stringList: ["one", "two"]
    property list<int> intList: [1, 2]
    property list<QtObject> objectList: [this, this]

    Component.onCompleted: {
        console.log(stringList)
        console.log(stringList + "")

        console.log(intList)
        console.log(intList + "")

        console.log(objectList)
        console.log(objectList + "")

        console.log(["a", "b"]);

        // TODO: Cannot do this, yet, because we cannot coerce a list to string on the fly.
        //       We need to store it as list first.
        // console.log(["a", "b"] + "");
    }
}
