pragma Strict
import QtQml

QtObject {
    objectName: {
        let a = "a";
        const b = "b";
        return a + b;
    }
}
