pragma Strict

import TestTypes
import QtQml

QtObject {
    property rect r: Qt.rect(1, 2, 3, 4)
    property point p: Qt.point(5, 6);
    property ObjectType o: ObjectType {
        id: oo
        objectName: "short"
        property int makeItASeparateType: 1
    }

    property withLength wr: r
    property withLength wp: p
    property withLength wi: 17
    property withLength wo: oo
    property withLength wz: ObjectType {
        objectName: "looooong"
        property int makeItASeparateType: 2
    }
}
