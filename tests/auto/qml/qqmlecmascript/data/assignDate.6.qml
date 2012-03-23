import Qt.test 1.0

MyTypeObject {
    dateProperty: if(1) new Date("1982-11-25")
    dateTimeProperty: if(1) new Date("2009-05-12T15:22:01+02:00")
}
