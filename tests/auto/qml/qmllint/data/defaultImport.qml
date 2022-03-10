import Things 6.0

QtObject {
    property var db: LocalStorage.openDatabaseSync("foo", "2", "bar", 4)
}
