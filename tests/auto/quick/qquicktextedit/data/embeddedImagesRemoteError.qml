import QtQuick

TextEdit {
    property string serverBaseUrl;
    textFormat: TextEdit.RichText
    text: "<img src='" + serverBaseUrl + "/notexists.png'>"
}
