import QtQml

QtObject {
    property alias theInner: anonymous

    property QtObject inner: QtObject {
        id: anonymous
        property int a: 5
    }
}

