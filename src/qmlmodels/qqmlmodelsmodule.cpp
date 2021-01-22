/****************************************************************************
**
** Copyright (C) 2016 Research In Motion.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qqmlmodelsmodule_p.h"
#include <private/qtqmlmodelsglobal_p.h>

#if QT_CONFIG(qml_list_model)
#include <private/qqmllistmodel_p.h>
#include <private/qqmllistmodelworkeragent_p.h>
#endif
#if QT_CONFIG(qml_delegate_model)
#include <private/qqmlabstractdelegatecomponent_p.h>
#include <private/qqmldelegatemodel_p.h>
#include <private/qquickpackage_p.h>
#include <private/qqmlcomponentattached_p.h>
#endif
#if QT_CONFIG(qml_object_model)
#include <private/qqmlobjectmodel_p.h>
#include <private/qqmlinstantiator_p.h>
#endif

QT_BEGIN_NAMESPACE

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

void QQmlModelsModule::registerQmlTypes()
{
    // Don't add anything here. These are only for backwards compatibility.
    // Don't convert these to qmlRegisterTypesAndRevisions!
    // -> the annotations in the headers are for the QtQml.Models module <-
#if QT_CONFIG(qml_object_model)
    qmlRegisterType<QQmlInstantiator>("QtQml", 2, 1, "Instantiator"); // Only available in >= 2.1
    qmlRegisterAnonymousType<QQmlInstanceModel>("QtQml", 2);
#endif
}

void QQmlModelsModule::registerQuickTypes()
{
    // Don't add anything here. These are only for backwards compatibility.
    // Don't convert these to qmlRegisterTypesAndRevisions!
    // -> the annotations in the headers are for the QtQml.Models module <-

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

QT_END_NAMESPACE
