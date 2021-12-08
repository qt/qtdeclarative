import QtQuick

ListModel {
    property var x: TypeDoesNotExist {
        property string s: DoesNotExistEither.value;
    }
}
