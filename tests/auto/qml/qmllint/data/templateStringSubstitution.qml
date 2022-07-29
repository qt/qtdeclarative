import QtQml

QtObject {
  property int i
  property int j: `hallo ${i}`
}
