import bindable 1.0

WithBinding {
    property int baseProp: 0
    BindableInterceptor on prop {}
}
