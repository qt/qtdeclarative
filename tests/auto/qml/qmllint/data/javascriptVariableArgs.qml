import QtQml

QtObject {
    function varArgs() {}
    Component.onCompleted: {
        console.log("It works!");
        varArgs("This works", 2);
    }
}
