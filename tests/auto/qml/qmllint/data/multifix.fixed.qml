pragma ComponentBehavior: Bound
import QtQml

QtObject {
   id: root

   property Component cursorDelegate: QtObject {
      objectName: root.objectName
   }

   property Component background: QtObject {
      objectName: root.objectName
   }
}
