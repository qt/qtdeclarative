import QtQuick 2.1 as MyQualifiedImport
import QtQuick 2.0

MyQualifiedImport.Item {
    id:shouldBeQtQuickItem

    Text {
        id: shouldBeQtQuickText0
    }

    TextEdit {}

    MyQualifiedImport.TextEdit {}

    Component {
        id: shouldBeQtQmlComponent

        Item {

            MyQualifiedImport.Text {
                id: shouldBeQtQuickText1
            }

            Text {
                id: shouldBeQtQuickText2
            }

            MyQualifiedImport.TextInput {
                id: shouldBeQtQuickTextInput3
            }

            Timer {
                id: indirectlyImportedFromQtQml
            }

            MyQualifiedImport.Timer {
                id: indirectlyImportedFromQtQml2
            }
        }
    }
}
