import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ApplicationWindow {
    width: 400
    height: 300
    title: "FileDialog Example"

    property alias dialog: dialog

    function doneAccepted() {
        dialog.done(FileDialog.Accepted)
    }

    function doneRejected() {
        dialog.done(FileDialog.Rejected)
    }

    FileDialog {
        id: dialog
        title: "Please choose a file"
        property string forceFolder
        fileMode: FileDialog.SaveFile
        onCurrentFolderChanged: {
            if (currentFolder != forceFolder) {
                currentFolder = forceFolder;
            }
        }
    }
}
