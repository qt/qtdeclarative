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

void QQmlModelsModule::defineModule()
{
    const char uri[] = "QtQml.Models";

#if QT_CONFIG(qml_list_model)
    qmlRegisterTypesAndRevisions<QQmlListElement, QQmlListModel, QQmlListModelWorkerAgent>(uri, 2);
#endif
#if QT_CONFIG(qml_delegate_model)
    // TODO: Get rid of these. It's called DelegateModel and DelegateModelGroup these days.
    qmlRegisterType<QQmlDelegateModel>(uri, 2, 0, "VisualDataModel");
    qmlRegisterType<QQmlDelegateModelGroup>(uri, 2, 0, "VisualDataGroup");

    qmlRegisterTypesAndRevisions<QQmlDelegateModel, QQmlDelegateModelGroup, QQuickPackage, QQmlAbstractDelegateComponent>(uri, 2);
#endif
#if QT_CONFIG(qml_object_model)
    qmlRegisterTypesAndRevisions<QQmlObjectModel, QQmlInstantiator, QQmlInstanceModel>(uri, 2);
#endif
#if QT_CONFIG(itemmodel)
    qmlRegisterTypesAndRevisions<QItemSelectionModelForeign>(uri, 2);
#endif
}

QT_END_NAMESPACE
