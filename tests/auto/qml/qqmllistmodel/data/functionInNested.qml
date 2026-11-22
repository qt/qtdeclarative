import QtQml

Instantiator {
    model: ListModel {
        ListElement {
            submenu: [
                ListElement {
                    func : function() {
                        console.log("called")
                    }
                }
            ]
        }
    }
    delegate: Instantiator {
        required property ListModel submenu
        model: submenu
        delegate: QtObject {
            required property var func
            Component.onCompleted: {
                func()
            }
        }
    }
}
