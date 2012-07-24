import QtQuick 2.0
import Qt.test.importOrderApi 1.0
import Qt.test.importOrderApi 1.0 as Namespace
import NamespaceAndType 1.0
import NamespaceAndType 1.0 as NamespaceAndType

QtObject {
    property bool success: false

    Component.onCompleted: {
        var s0 = Data.value === 37 && Namespace.Data.value === 37 && Data.value === Namespace.Data.value;
        var s1 = NamespaceAndType.value === NamespaceAndType.NamespaceAndType.value &&
                 NamespaceAndType.value === 37 &&
                 NamespaceAndType.NamespaceAndType.value === 37;
        success = (s0 === true) && (s1 === true);
    }
}
