import QtQml

QtObject {
    property int count: 0
    Component.onCompleted: {
        ++count;
    }
}
