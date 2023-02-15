pragma Strict
import QtQuick

Item {
    property url emptyUrl: ""
    property url sourceUrl: "some/path/file.png"

    property bool emptyUrlStrict: emptyUrl === Qt.resolvedUrl("")
    property bool emptyUrlWeak: emptyUrl == Qt.resolvedUrl("")

    property bool sourceUrlStrict: sourceUrl === Qt.url("some/path/file.png");
    property bool sourceUrlWeak: sourceUrl == Qt.url("some/path/file.png");

    property bool sourceIsNotEmptyStrict: sourceUrl !== emptyUrl
    property bool sourceIsNotEmptyWeak: sourceUrl != emptyUrl
}
