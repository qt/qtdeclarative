import QtQml 2.9

QtObject {
    property Timer stuff: Component {
        QtObject {
            objectName: "wrong"
        }
    }
}
