import QmltcTests 1.0

MainComponent {
    firstComponent.setMe: true

    MainComponent {
        firstComponent.setMe: true
    }
}
