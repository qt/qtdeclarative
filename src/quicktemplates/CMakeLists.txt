# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QuickTemplates2 Module:
#####################################################################

qt_internal_add_qml_module(QuickTemplates2
    URI "QtQuick.Templates"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuickTemplates2Plugin
    DEPENDENCIES
        QtQuick/auto
    PLUGIN_TARGET qtquicktemplates2plugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
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
    NO_UNITY_BUILD_SOURCES
        qquickpopupitem.cpp # redefinition of 'contentItemName' (from qquickcontrol.cpp)
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
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

qt_internal_extend_target(QuickTemplates2 CONDITION TARGET Qt::QmlModels
    LIBRARIES
        Qt::QmlModelsPrivate
    PUBLIC_LIBRARIES
        Qt::QmlModels
    PRIVATE_MODULE_INTERFACE
        Qt::QmlModelsPrivate
)

qt_internal_extend_target(QuickTemplates2 CONDITION QT_FEATURE_accessibility
    SOURCES
        accessible/qaccessiblequickpage.cpp accessible/qaccessiblequickpage_p.h
)

qt_internal_extend_target(QuickTemplates2 CONDITION QT_FEATURE_quick_tableview
    SOURCES
        qquickheaderview.cpp qquickheaderview_p.h
        qquickheaderview_p_p.h
        qquickselectionrectangle.cpp qquickselectionrectangle_p.h
        qquickselectionrectangle_p_p.h
)

qt_internal_extend_target(QuickTemplates2 CONDITION QT_FEATURE_quick_treeview
    SOURCES
        qquicktreeviewdelegate.cpp qquicktreeviewdelegate_p.h
)

qt_internal_extend_target(QuickTemplates2 CONDITION QT_FEATURE_quick_listview AND QT_FEATURE_quick_pathview
    SOURCES
        qquicktumbler.cpp qquicktumbler_p.h
        qquicktumbler_p_p.h
)

qt_internal_extend_target(QuickTemplates2 CONDITION QT_FEATURE_quicktemplates2_calendar
    SOURCES
        qquickcalendar.cpp qquickcalendar_p.h
        qquickcalendarmodel.cpp qquickcalendarmodel_p.h
        qquickdayofweekmodel.cpp qquickdayofweekmodel_p.h
        qquickdayofweekrow.cpp qquickdayofweekrow_p.h
        qquickmonthgrid.cpp qquickmonthgrid_p.h
        qquickmonthmodel.cpp qquickmonthmodel_p.h
        qquickweeknumbercolumn.cpp qquickweeknumbercolumn_p.h
        qquickweeknumbermodel.cpp qquickweeknumbermodel_p.h
)

qt_internal_extend_target(QuickTemplates2 CONDITION QT_FEATURE_quick_viewtransitions
    SOURCES
        qquickstacktransition.cpp
        qquickstacktransition_p_p.h
)

qt_internal_extend_target(QuickTemplates2 CONDITION QT_FEATURE_qml_delegate_model
    SOURCES
        qquickcombobox.cpp
        qquickcombobox_p.h
)

qt_internal_extend_Target(qtquicktemplates2plugin
    SOURCES
        qtquicktemplates2plugin.cpp
    LIBRARIES
        Qt::Quick
        Qt::QuickTemplates2Private
)
