import QtQml
import Test

DelegateModel {
    id: root

    property UndefinedRoleModel undefinedRoleModel: UndefinedRoleModel {}

    model: undefinedRoleModel

    delegate: QtObject {
        required property string bar
    }
}
