import QualifiedNamesTests 6.0
import QualifiedNamesTests 5.0 as MyQualifiedImport
import QtQuick 2.0

Item {
    // A {} <- QML_REMOVED_IN_VERSION(6, 0)

    B {}

    D {}

    MyQualifiedImport.A {}
    MyQualifiedImport.B {}
}
