Item {
    Component.onCompleted: {
        // Make sure that nested if statements get properly braced
        if (a) {
            if (b)
                foo();
            else
                bar();
        } else if (x == 3) {
            stuff();
        } else {
            foo_bar();
        }
        // Same for "for"
        if (a) {
            for (x in y) {
                bar();
                y();
            }
        }
        // ...and while
        if (b) {
            while (y in x) {
                foo();
                x();
            }
        }
    }
}
