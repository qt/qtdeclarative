Item {

    Button {
    }

    Button {
        id: foo
    }

    height: 360
    width: 360

    Rectangle {
        color: "salmon"
        height: 360
        width: 360

        Item {

            Loader {
                height: 360
                width: 360
            }
            Loader {
                height: 360
                width: 360
            }

        }
    }
    Image {
        height: 360
        width: 360
    }
}
