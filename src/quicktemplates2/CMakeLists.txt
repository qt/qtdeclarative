# Generated from quicktemplates2.pro.

#####################################################################
## QuickTemplates2 Module:
#####################################################################

qt_internal_add_module(QuickTemplates2
    GENERATE_METATYPES
    SOURCES
        accessible/qaccessiblequickpage.cpp accessible/qaccessiblequickpage_p.h
        qquickabstractbutton.cpp qquickabstractbutton_p.h
        qquickabstractbutton_p_p.h
        qquickaction.cpp qquickaction_p.h
        qquickactiongroup.cpp qquickactiongroup_p.h
        qquickapplicationwindow.cpp qquickapplicationwindow_p.h
        qquickbusyindicator.cpp qquickbusyindicator_p.h
        qquickbutton.cpp qquickbutton_p.h
        qquickbutton_p_p.h
        qquickbuttongroup.cpp qquickbuttongroup_p.h
        qquickcheckbox.cpp qquickcheckbox_p.h
        qquickcheckdelegate.cpp qquickcheckdelegate_p.h
        qquickcombobox.cpp qquickcombobox_p.h
        qquickcontainer.cpp qquickcontainer_p.h
        qquickcontainer_p_p.h
        qquickcontentitem.cpp qquickcontentitem_p.h
        qquickcontrol.cpp qquickcontrol_p.h
        qquickcontrol_p_p.h
        qquickdeferredexecute.cpp
        qquickdeferredexecute_p_p.h
        qquickdeferredpointer_p_p.h
        qquickdelaybutton.cpp qquickdelaybutton_p.h
        qquickdial.cpp qquickdial_p.h
        qquickdialog.cpp qquickdialog_p.h
        qquickdialog_p_p.h
        qquickdialogbuttonbox.cpp qquickdialogbuttonbox_p.h
        qquickdialogbuttonbox_p_p.h
        qquickdrawer.cpp qquickdrawer_p.h
        qquickdrawer_p_p.h
        qquickframe.cpp qquickframe_p.h
        qquickframe_p_p.h
        qquickgroupbox.cpp qquickgroupbox_p.h
        qquickheaderview.cpp qquickheaderview_p.h
        qquickheaderview_p_p.h
        qquickicon.cpp qquickicon_p.h
        qquickindicatorbutton_p.cpp qquickindicatorbutton_p.h
        qquickitemdelegate.cpp qquickitemdelegate_p.h
        qquickitemdelegate_p_p.h
        qquicklabel.cpp qquicklabel_p.h
        qquicklabel_p_p.h
        qquickmenu.cpp qquickmenu_p.h
        qquickmenu_p_p.h
        qquickmenubar.cpp qquickmenubar_p.h
        qquickmenubar_p_p.h
        qquickmenubaritem.cpp qquickmenubaritem_p.h
        qquickmenubaritem_p_p.h
        qquickmenuitem.cpp qquickmenuitem_p.h
        qquickmenuitem_p_p.h
        qquickmenuseparator.cpp qquickmenuseparator_p.h
        qquickoverlay.cpp qquickoverlay_p.h
        qquickoverlay_p_p.h
        qquickpage.cpp qquickpage_p.h
        qquickpage_p_p.h
        qquickpageindicator.cpp qquickpageindicator_p.h
        qquickpane.cpp qquickpane_p.h
        qquickpane_p_p.h
        qquickpopup.cpp qquickpopup_p.h
        qquickpopup_p_p.h
        qquickpopupanchors.cpp qquickpopupanchors_p.h
        qquickpopupanchors_p_p.h
        qquickpopupitem.cpp
        qquickpopupitem_p_p.h
        qquickpopuppositioner.cpp
        qquickpopuppositioner_p_p.h
        qquickpresshandler.cpp
        qquickpresshandler_p_p.h
        qquickprogressbar.cpp qquickprogressbar_p.h
        qquickradiobutton.cpp qquickradiobutton_p.h
        qquickradiodelegate.cpp qquickradiodelegate_p.h
        qquickrangeslider.cpp qquickrangeslider_p.h
        qquickroundbutton.cpp qquickroundbutton_p.h
        qquickscrollbar.cpp qquickscrollbar_p.h
        qquickscrollbar_p_p.h
        qquickscrollindicator.cpp qquickscrollindicator_p.h
        qquickscrollview.cpp qquickscrollview_p.h
        qquickshortcutcontext.cpp
        qquickshortcutcontext_p_p.h
        qquickslider.cpp qquickslider_p.h
        qquickspinbox.cpp qquickspinbox_p.h
        qquicksplitview.cpp qquicksplitview_p.h
        qquickstackelement.cpp
        qquickstackelement_p_p.h
        qquickstacktransition.cpp
        qquickstacktransition_p_p.h
        qquickstackview.cpp qquickstackview_p.cpp qquickstackview_p.h
        qquickstackview_p_p.h
        qquickswipe_p.h
        qquickswipedelegate.cpp qquickswipedelegate_p.h
        qquickswipedelegate_p_p.h
        qquickswipeview.cpp qquickswipeview_p.h
        qquickswitch.cpp qquickswitch_p.h
        qquickswitchdelegate.cpp qquickswitchdelegate_p.h
        qquicktabbar.cpp qquicktabbar_p.h
        qquicktabbutton.cpp qquicktabbutton_p.h
        qquicktextarea.cpp qquicktextarea_p.h
        qquicktextarea_p_p.h
        qquicktextfield.cpp qquicktextfield_p.h
        qquicktextfield_p_p.h
        qquicktheme.cpp qquicktheme_p.h
        qquicktheme_p_p.h
        qquicktoolbar.cpp qquicktoolbar_p.h
        qquicktoolbutton.cpp qquicktoolbutton_p.h
        qquicktoolseparator.cpp qquicktoolseparator_p.h
        qquicktooltip.cpp qquicktooltip_p.h
        qquickvelocitycalculator.cpp
        qquickvelocitycalculator_p_p.h
        qtquicktemplates2global.cpp qtquicktemplates2global_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:quicktemplates2.pro:<TRUE>:
# MODULE = "quicktemplates2"
# QMLTYPES_FILENAME = "plugins.qmltypes"
# QMLTYPES_INSTALL_DIR = "$$[QT_INSTALL_QML]/QtQuick/Templates"
# QML_IMPORT_NAME = "QtQuick.Templates"
# QML_IMPORT_VERSION = "$$QT_VERSION"

## Scopes:
#####################################################################

qt_internal_extend_target(QuickTemplates2 CONDITION TARGET Qt::QmlModels
    LIBRARIES
        Qt::QmlModelsPrivate
    PUBLIC_LIBRARIES
        Qt::QmlModels
    PRIVATE_MODULE_INTERFACE
        Qt::QmlModelsPrivate
)

qt_internal_extend_target(QuickTemplates2 CONDITION QT_FEATURE_quick_listview AND QT_FEATURE_quick_pathview
    SOURCES
        qquicktumbler.cpp qquicktumbler_p.h
        qquicktumbler_p_p.h
)

set_target_properties(QuickTemplates2 PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI QtQuick.Templates
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtQuick/Templates"
)

qt6_qml_type_registration(QuickTemplates2)
