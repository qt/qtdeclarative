TEMPLATE = subdirs
QT_FOR_CONFIG += qml

qtHaveModule(quick) {
    SUBDIRS += \
        qmlextensionplugins

    qtConfig(qml-network): \
        SUBDIRS += networkaccessmanagerfactory
}

SUBDIRS += \
          referenceexamples \
          tutorials

EXAMPLE_FILES = \
    dynamicscene \
    qml-i18n \
    locale
