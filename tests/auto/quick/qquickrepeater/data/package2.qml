import QtQuick 2.0
import QtQml.Models 2.2
import QtQuick.Window 2.0

Item {
    width: 300
    height: 300
    visible: true    
    DelegateModel {
        id: mdl

        model: 1
        delegate: Package {
            Item {
                id: first
                Package.name: "first"
            }
            Item{
                id: second
                Package.name: "second"
            }
        }
    }

    Repeater {
        model: mdl.parts.first
    }
    Repeater {
        model: mdl.parts.second
    }

    function setup():  bool {
        mdl.model = 2
        return true;
    }
}
