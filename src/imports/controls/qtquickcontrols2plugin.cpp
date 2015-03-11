/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQml/qqmlextensionplugin.h>
#include "qquickstyle_p.h"

#include <QtQuickControls/private/qquickabstractapplicationwindow_p.h>
#include <QtQuickControls/private/qquickabstractbusyindicator_p.h>
#include <QtQuickControls/private/qquickabstractbutton_p.h>
#include <QtQuickControls/private/qquickabstractcheckbox_p.h>
#include <QtQuickControls/private/qquickabstractframe_p.h>
#include <QtQuickControls/private/qquickabstractgroupbox_p.h>
#include <QtQuickControls/private/qquickabstractpageindicator_p.h>
#include <QtQuickControls/private/qquickabstractprogressbar_p.h>
#include <QtQuickControls/private/qquickabstractradiobutton_p.h>
#include <QtQuickControls/private/qquickabstractscrollbar_p.h>
#include <QtQuickControls/private/qquickabstractscrollindicator_p.h>
#include <QtQuickControls/private/qquickabstractslider_p.h>
#include <QtQuickControls/private/qquickabstractspinbox_p.h>
#include <QtQuickControls/private/qquickabstractstackview_p.h>
#include <QtQuickControls/private/qquickabstractswitch_p.h>
#include <QtQuickControls/private/qquickabstracttabbar_p.h>
#include <QtQuickControls/private/qquickabstracttabbutton_p.h>
#include <QtQuickControls/private/qquickabstracttabview_p.h>
#include <QtQuickControls/private/qquickabstracttextarea_p.h>
#include <QtQuickControls/private/qquickabstracttextfield_p.h>
#include <QtQuickControls/private/qquickabstracttogglebutton_p.h>
#include <QtQuickControls/private/qquickabstracttoolbar_p.h>

#include <QtQuickControls/private/qquickcontrol_p.h>
#include <QtQuickControls/private/qquickexclusivegroup_p.h>

void initResources()
{
    Q_INIT_RESOURCE(qtquickcontrols2plugin);
}

QT_BEGIN_NAMESPACE

class QtQuickControls2Plugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface/1.0")

public:
    void registerTypes(const char *uri);
    void initializeEngine(QQmlEngine *engine, const char *uri);
};

void QtQuickControls2Plugin::registerTypes(const char *uri)
{
    qmlRegisterType<QQuickAbstractApplicationWindow>(uri, 2, 0, "AbstractApplicationWindow");
    qmlRegisterType<QQuickAbstractBusyIndicator>(uri, 2, 0, "AbstractBusyIndicator");
    qmlRegisterType<QQuickAbstractButton>(uri, 2, 0, "AbstractButton");
    qmlRegisterType<QQuickAbstractCheckBox>(uri, 2, 0, "AbstractCheckBox");
    qmlRegisterType<QQuickAbstractFrame>(uri, 2, 0, "AbstractFrame");
    qmlRegisterType<QQuickAbstractGroupBox>(uri, 2, 0, "AbstractGroupBox");
    qmlRegisterType<QQuickAbstractPageIndicator>(uri, 2, 0, "AbstractPageIndicator");
    qmlRegisterType<QQuickAbstractProgressBar>(uri, 2, 0, "AbstractProgressBar");
    qmlRegisterType<QQuickAbstractRadioButton>(uri, 2, 0, "AbstractRadioButton");
    qmlRegisterType<QQuickAbstractScrollBar>(uri, 2, 0, "AbstractScrollBar");
    qmlRegisterType<QQuickAbstractScrollIndicator>(uri, 2, 0, "AbstractScrollIndicator");
    qmlRegisterType<QQuickAbstractSlider>(uri, 2, 0, "AbstractSlider");
    qmlRegisterType<QQuickAbstractSpinBox>(uri, 2, 0, "AbstractSpinBox");
    qmlRegisterType<QQuickAbstractStackView>(uri, 2, 0, "AbstractStackView");
    qmlRegisterType<QQuickAbstractSwitch>(uri, 2, 0, "AbstractSwitch");
    qmlRegisterType<QQuickAbstractTabBar>(uri, 2, 0, "AbstractTabBar");
    qmlRegisterType<QQuickAbstractTabButton>(uri, 2, 0, "AbstractTabButton");
    qmlRegisterType<QQuickAbstractTabView>(uri, 2, 0, "AbstractTabView");
    qmlRegisterType<QQuickAbstractTextArea>(uri, 2, 0, "AbstractTextArea");
    qmlRegisterType<QQuickAbstractTextField>(uri, 2, 0, "AbstractTextField");
    qmlRegisterType<QQuickAbstractToggleButton>(uri, 2, 0, "AbstractToggleButton");
    qmlRegisterType<QQuickAbstractToolBar>(uri, 2, 0, "AbstractToolBar");

    qmlRegisterUncreatableType<QQuickExclusiveAttached>(uri, 2, 0, "Exclusive", "Exclusive is an attached property");
    qmlRegisterUncreatableType<QQuickStackAttached>(uri, 2, 0, "Stack", "Stack is an attached property");
    qmlRegisterUncreatableType<QQuickStyle>(uri, 2, 0, "Style", "Style is an attached property");
    qmlRegisterUncreatableType<QQuickTabAttached>(uri, 2, 0, "Tab", "Tab is an attached property");

    qmlRegisterType<QQuickControl>(uri, 2, 0, "Control");
    qmlRegisterType<QQuickExclusiveGroup>(uri, 2, 0, "ExclusiveGroup");

    qmlRegisterRevision<QQuickTextInput, 6>(uri, 2, 0);
    qmlRegisterRevision<QQuickTextEdit, 6>(uri, 2, 0);
}

void QtQuickControls2Plugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(engine);
    Q_UNUSED(uri);
    initResources();
}

QT_END_NAMESPACE

#include "qtquickcontrols2plugin.moc"
