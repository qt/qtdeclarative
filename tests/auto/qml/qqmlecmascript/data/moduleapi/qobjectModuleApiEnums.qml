import QtQuick 2.0
import Qt.test.qobjectApi 1.0 as QtTestQObjectApi               // qobject module API installed into new uri

QtObject {
    property int enumValue: QtTestQObjectApi.EnumValue2;
    property int enumMethod: QtTestQObjectApi.qobjectEnumTestMethod(QtTestQObjectApi.EnumValue1);
}

