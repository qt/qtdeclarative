import QtQml

QtObject {
  id: root
  default property list<QtObject> data: [ QtObject {objectName: "1" } ]
  property QtObject entry: data[data.length - 1]

}
