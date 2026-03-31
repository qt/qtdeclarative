pragma Strict
import QtQml

QtObject {
    id: self

    property string hello: "Hello"
    property string result

    function greet(prefix: string, suffix: string) : string {
        return prefix + self.hello + suffix
    }

    Component.onCompleted: {
        self.result = self.greet("[", "]")
    }
}
