import QtQuick 2.0
import qqmllanguage.Test 1.0

QtObject {
    property alias otherAlias: otherObject

    property variant other
    other: MyQmlObject {
        id: otherObject
        value: 10
    }
}

