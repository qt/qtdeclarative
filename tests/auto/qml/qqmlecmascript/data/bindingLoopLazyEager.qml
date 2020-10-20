import test

BindingLoop {
    eager1: value+1
    value: eager1+1
}
