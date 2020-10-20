import test

BindingLoop {
    eager1: oldprop+1
    oldprop: eager1+1
}
