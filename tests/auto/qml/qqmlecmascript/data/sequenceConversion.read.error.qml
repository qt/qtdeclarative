import QtQuick 2.0
import Qt.test 1.0

Item {
    id: root
    objectName: "root"

    MySequenceConversionObject {
        id: msco
        objectName: "msco"
    }

    property int pointListLength: 0
    property variant pointList

    function performTest() {
        // we have NOT registered QList<QPoint> as a type
        pointListLength = msco.pointListProperty.length;
        pointList = msco.pointListProperty;
    }
}
