import QtQml 2.0

QtObject {
    property string hello: "Hello"
    property string greeting: hello + ", World!"

    function jsGetGreeting() {
        return hello + ", World!";
    }

    property QtObject child: QtObject {
        property string hello: "Hello"
        property string greeting: hello + ", Qt!"

        function jsGetGreeting() {
            return hello + ", Qt!";
        }

        property QtObject child: QtObject {
            property string hello: "Hello"
            property string greeting: hello + ", Qml!"

            function jsGetGreeting() {
                return hello + ", Qml!";
            }
        }
    }
}
