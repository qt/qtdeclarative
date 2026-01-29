import QtQml
import Test

Instantiator {
    objectName: object?.objectName ?? "none"

    model: ChangingModel {}
    delegate: DelegateChooser {
        role: "theRole"

        DelegateChoice {
            roleValue: "Foo"
            delegate: QtObject {
                objectName: model.theRole
            }
        }

        DelegateChoice {
            roleValue: "Bla"
            delegate: QtObject {
                objectName: model.theRole
            }
        }
    }
}
