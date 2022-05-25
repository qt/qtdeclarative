import QtQuick

// relative import: has to work when loading from android assets by path or by URL
import "pages"

Window {
  width: 640
  height: 480
  visible: true
  title: qsTr("Hello World")

  MainPage { }
}
