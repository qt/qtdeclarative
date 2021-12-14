# Generated from qml.pro.

qt_internal_add_example(dynamicscene)
add_subdirectory(referenceexamples)
add_subdirectory(tutorials)
qt_internal_add_example(shell)
if(TARGET Qt::Quick)
    qt_internal_add_example(qmlextensionplugins)
    qt_internal_add_example(xmlhttprequest)
endif()
if(QT_FEATURE_qml_network AND TARGET Qt::Quick)
    qt_internal_add_example(networkaccessmanagerfactory)
endif()
