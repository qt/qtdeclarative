import QtQml

QtObject {
    property string foo: true ? eval() : ""
    property string bar: {
        if (true) {
            eval()
        } else {
        }
    }

    property string baz: {
        if (true) {
            eval()
        }
    }
}
