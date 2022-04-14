set(cpp_sources
    # attached types:
    cpptypes/testattachedtype.h cpptypes/testattachedtype.cpp
    # grouped types:
    cpptypes/testgroupedtype.h cpptypes/testgroupedtype.cpp
    # private properties:
    cpptypes/testprivateproperty.h cpptypes/testprivateproperty.cpp
    cpptypes/private/testprivateproperty_p.h

    cpptypes/typewithproperties.h cpptypes/typewithproperties.cpp
    # deferred:
    cpptypes/deferredpropertytypes.h cpptypes/deferredpropertytypes.cpp
)

set(qml_sources
    HelloWorld.qml
    NameConflict.qml
    simpleQtQuickTypes.qml
    typeWithEnums.qml
    methods.qml
    properties.qml
    ObjectWithId.qml
    documentWithIds.qml
    importNamespace.qml
    ComponentType.qml
    componentTypes.qml
    gradients.qml
    qjsvalueAssignments.qml

    signalHandlers.qml
    javaScriptFunctions.qml
    changingBindings.qml
    propertyAlias.qml
    propertyAlias_external.qml
    propertyChangeHandler.qml
    NestedHelloWorld.qml
    ComponentHelloWorld.qml
    listProperty.qml
    listPropertySameName.qml
    defaultProperty.qml
    defaultPropertyCorrectSelection.qml
    # defaultAlias.qml
    propertyReturningFunction.qml
    attachedProperty.qml
    groupedProperty.qml
    groupedProperty_qquicktext.qml
    localImport.qml
    localImport_explicit.qml
    newPropertyBoundToOld.qml
    oldPropertyBoundToNew.qml
    nonLocalQmlPropertyBoundToAny.qml
    localDerived.qml
    justAnimation.qml
    justAnimationOnAlias.qml
    behaviorAndAnimation.qml
    behaviorAndAnimationOnAlias.qml
    singletonUser.qml
    bindingsThroughIds.qml
    localImport_context.qml
    neighbors_context.qml
    delegate_context.qml
    nontrivial_context.qml
    javascriptCaller.qml
    listView.qml
    bindingOnValueType.qml
    keyEvents.qml
    complexAliases.qml
    PrivateProperty.qml
    privatePropertySubclass.qml
    calqlatrBits.qml
    propertyChangeAndSignalHandlers.qml
    deferredProperties.qml
    deferredProperties_group.qml
    deferredProperties_attached.qml
    deferredProperties_complex.qml

    # support types:
    DefaultPropertySingleChild.qml
    DefaultPropertyManyChildren.qml
    LocallyImported.qml
    LocalWithOnCompleted.qml
    LocallyImported_context.qml
    # SingletonThing.qml
)

set(js_sources
    subfolder/code.js
)

set(common_libraries
    Qt::Core
    Qt::QmlPrivate
    Qt::QuickPrivate
    Qt::TestPrivate
    Qt::Gui # QColor, QMatrix4x4, ...
)

set_source_files_properties(NameConflict.qml PROPERTIES
    QT_QMLTC_FILE_BASENAME ResolvedNameConflict)

qt_add_library(qmltc_test_module STATIC)
qt_autogen_tools_initial_setup(qmltc_test_module)

# use PUBLIC everywhere to simplify the build of the test binary
target_include_directories(qmltc_test_module PUBLIC cpptypes/)
target_link_libraries(qmltc_test_module PUBLIC ${common_libraries})

qt6_add_qml_module(qmltc_test_module
    VERSION 1.0
    URI QmltcTests
    SOURCES
        ${cpp_sources}
    QML_FILES
        ${qml_sources}
        ${js_sources}
    DEPENDENCIES
        QtQuick
)
qt_internal_target_compile_qml_to_cpp(qmltc_test_module
    NAMESPACE QmltcTest
    QML_FILES
        ${qml_sources}
)

qt_autogen_tools_initial_setup(qmltc_test_moduleplugin)
