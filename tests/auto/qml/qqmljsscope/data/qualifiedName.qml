import QualifiedNamesTests 6.0
import QualifiedNamesTests 5.0 as MyQualifiedImport
import QtQuick 2.0

Item {
    A {}

    B {}

    D {}

    MyQualifiedImport.A {}
    MyQualifiedImport.B {}
}
