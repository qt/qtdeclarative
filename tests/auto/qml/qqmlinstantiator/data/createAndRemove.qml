import QtQml 2.1
import QtQuick 2.1
import Test 1.0

Rectangle {
    Instantiator {
        objectName: "instantiator1"
        model: Model1
        delegate: QtObject {
            property string datum: model.text
        }
    }
    Component.onCompleted: {
        Model1.add("Delta");
        Model1.add("Gamma");
        Model1.add("Beta");
        Model1.add("Alpha");
    }
}
