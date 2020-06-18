Item {
    function test() {
        //// The following if blocks should NOT HAVE braces
        // Single branch, no braces
        if (true)
            console.log("foo");

        // Single branch, no braces
        if (true)
            console.log("foo");

        // Multiple branches, No braces
        if (true)
            console.log("foo");
        else if (false)
            console.log("bar");
        else
            console.log("baz");
        // Multiple branches, all braces
        if (true)
            console.log("foo");
        else if (false)
            console.log("bar");
        else
            console.log("baz");
        //// The following if blocks should HAVE braces
        // Single branch, braces
        if (true) {
            console.log("foo");
            console.log("bar");
        }
        // Multiple branches, some braces
        if (true) {
            console.log("foo");
            console.log("foo2");
        } else if (false) {
            console.log("bar");
        } else {
            console.log("baz");
        }
        // Multiple branches, some braces
        if (true) {
            console.log("foo");
        } else if (false) {
            console.log("bar");
            console.log("bar2");
        } else {
            console.log("baz");
        }
        // Multiple branches, some braces
        if (true) {
            console.log("foo");
        } else if (false) {
            console.log("bar");
        } else {
            console.log("baz");
            console.log("baz2");
        }
    }

}
