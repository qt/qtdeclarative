import test

BindingLoop {
    eager1: eager2+1
    eager2: eager1+1
}
