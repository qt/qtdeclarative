import QtQml

Binding {
    property string input
    property string output
    onInputChanged: output = input
}
