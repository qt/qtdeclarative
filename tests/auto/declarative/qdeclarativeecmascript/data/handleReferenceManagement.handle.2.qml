import QtQuick 2.0
import Qt.test 1.0

Item {
    id: obj
    objectName: "obj"
    property CircularReferenceHandle first
    property CircularReferenceHandle second

    CircularReferenceHandle {
        id: crh
        objectName: "crh"
    }

    function circularReference() {
        // generate the circularly referential pair
        first = crh.generate(crh);
        second = crh.generate(crh);
        // note: must manually reparent in unit test
        // after setting the handle references.
    }
}
