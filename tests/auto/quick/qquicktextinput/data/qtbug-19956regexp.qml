import QtQuick 2.14

TextInput {
    id: textinput
    property variant regexvalue
    height: 50
    width: 200
    text: "abc"
    validator: RegularExpressionValidator {
        id: regexpvalidator
        regularExpression: regexvalue
    }
}
