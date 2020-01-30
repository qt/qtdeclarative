QtObject {
    small1: 3
    small2: foo
    // THIS NEEDS TO BE LAST
    largeBinding: {
        var x = 300;
        console.log(x);
    }
}
