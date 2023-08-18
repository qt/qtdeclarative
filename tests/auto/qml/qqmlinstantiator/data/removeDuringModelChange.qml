import QtQuick 2.15
import QtQml.Models 2.15

Instantiator {
    delegate: QtObject {
        function deactivate() {
            model.active = false;
        }
    }
}
