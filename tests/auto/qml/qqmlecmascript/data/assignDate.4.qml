import Qt.test 1.0

MyTypeObject {
    dateProperty: if(1) new Date("1982-11-25Z")
    dateTimeProperty: if(1) new Date("2009-05-12T13:22:01Z")
}
