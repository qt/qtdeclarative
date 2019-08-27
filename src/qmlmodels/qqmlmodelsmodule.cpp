/****************************************************************************
**
** Copyright (C) 2016 Research In Motion.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlmodelsmodule_p.h"
#include <private/qtqmlmodelsglobal_p.h>

#if QT_CONFIG(itemmodel)
#include <QtCore/qitemselectionmodel.h>
#endif
#if QT_CONFIG(qml_list_model)
#include <private/qqmllistmodel_p.h>
#endif
#if QT_CONFIG(qml_delegate_model)
#include <private/qqmldelegatemodel_p.h>
#include <private/qqmldelegatecomponent_p.h>
#include <private/qquickpackage_p.h>
#endif
#if QT_CONFIG(qml_object_model)
#include <private/qqmlobjectmodel_p.h>
#include <private/qqmlinstantiator_p.h>
#endif
#if QT_CONFIG(qml_table_model)
#include <private/qqmltablemodel_p.h>
#include <private/qqmltablemodelcolumn_p.h>
#endif

QT_BEGIN_NAMESPACE

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

void QQmlModelsModule::registerQmlTypes()
{
    // Don't add anything here. These are only for backwards compatibility.
#if QT_CONFIG(qml_object_model)
    qmlRegisterType<QQmlInstantiator>("QtQml", 2, 1, "Instantiator"); // Only available in >= 2.1
    qmlRegisterAnonymousType<QQmlInstanceModel>("QtQml", 2);
#endif
}

void QQmlModelsModule::registerQuickTypes()
{
    // Don't add anything here. These are only for backwards compatibility.

    const char uri[] = "QtQuick";

#if QT_CONFIG(qml_object_model)
    qmlRegisterType<QQmlInstantiator>(uri, 2, 1, "Instantiator");
    qmlRegisterAnonymousType<QQmlInstanceModel>(uri, 2);
    qmlRegisterType<QQmlObjectModel>(uri, 2, 0, "VisualItemModel");
#endif
#if QT_CONFIG(qml_list_model)
    qmlRegisterType<QQmlListElement>(uri, 2, 0, "ListElement");
    qmlRegisterCustomType<QQmlListModel>(uri, 2, 0, "ListModel", new QQmlListModelParser);
#endif
#if QT_CONFIG(qml_delegate_model)
    qmlRegisterType<QQmlDelegateModel>(uri, 2, 0, "VisualDataModel");
    qmlRegisterType<QQmlDelegateModelGroup>(uri, 2, 0, "VisualDataGroup");
    qmlRegisterType<QQuickPackage>(uri, 2, 0, "Package");
#endif
}

#endif // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

void QQmlModelsModule::defineModule()
{
    const char uri[] = "QtQml.Models";

#if QT_CONFIG(qml_list_model)
    qmlRegisterType<QQmlListElement>(uri, 2, 1, "ListElement");
    qmlRegisterCustomType<QQmlListModel>(uri, 2, 1, "ListModel", new QQmlListModelParser);
#endif
#if QT_CONFIG(qml_delegate_model)
    qmlRegisterType<QQmlDelegateModel>(uri, 2, 1, "DelegateModel");
    qmlRegisterType<QQmlDelegateModelGroup>(uri, 2, 1, "DelegateModelGroup");
    qmlRegisterType<QQuickPackage>(uri, 2, 14, "Package");
#endif
#if QT_CONFIG(qml_object_model)
    qmlRegisterType<QQmlObjectModel>(uri, 2, 1, "ObjectModel");
    qmlRegisterType<QQmlObjectModel,3>(uri, 2, 3, "ObjectModel");
    qmlRegisterType<QQmlInstantiator>(uri, 2, 14, "Instantiator");
    qmlRegisterAnonymousType<QQmlInstanceModel>(uri, 2);
#endif
#if QT_CONFIG(itemmodel)
    qmlRegisterType<QItemSelectionModel>(uri, 2, 2, "ItemSelectionModel");
#endif
}

void QQmlModelsModule::defineLabsModule()
{
    const char uri[] = "Qt.labs.qmlmodels";

#if QT_CONFIG(qml_delegate_model)
    qmlRegisterUncreatableType<QQmlAbstractDelegateComponent>(uri, 1, 0, "AbstractDelegateComponent", QQmlAbstractDelegateComponent::tr("Cannot create instance of abstract class AbstractDelegateComponent."));
    qmlRegisterType<QQmlDelegateChooser>(uri, 1, 0, "DelegateChooser");
    qmlRegisterType<QQmlDelegateChoice>(uri, 1, 0, "DelegateChoice");
#endif
#if QT_CONFIG(qml_table_model)
    qmlRegisterType<QQmlTableModel>(uri, 1, 0, "TableModel");
    qmlRegisterType<QQmlTableModelColumn>(uri, 1, 0, "TableModelColumn");
#endif
}

QT_END_NAMESPACE
