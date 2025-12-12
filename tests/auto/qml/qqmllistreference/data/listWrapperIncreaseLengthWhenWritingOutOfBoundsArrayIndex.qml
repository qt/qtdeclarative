import QtQml

QtObject {
    property list<QtObject> listWrapper: []

    property int expectedLength: 14;
    property int actualLength: 0;

    Component.onCompleted: {
        listWrapper[expectedLength - 1] = this

        actualLength = listWrapper.length
    }
}
