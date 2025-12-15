import QtQml
import "." as Custom

QtObject {
   property bool success: custom.customValue === "okay"

   property var v: Custom.Timer {
       id: custom
   }
}
