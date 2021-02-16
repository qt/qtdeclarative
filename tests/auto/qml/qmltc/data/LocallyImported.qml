import QtQuick

// TODO: this has to be a QQuickItem if we want to use QQuickItem derived
// children e.g. Text/Rectangle QML types
Item {
    property string baseMessage: "base"
    property string message: "base.message"
    property int count: 0
    property QtObject baseObject: QtObject {
        // this one would be deleted
    }
    default property list<QtObject> baseDefaultList

    Component.onCompleted: {
        ++count;
    }
}
