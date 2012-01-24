/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QtQuick1/private/qdeclarativeutilmodule_p.h"
#include "QtQuick1/private/qdeclarativeanimation_p.h"
#include "QtQuick1/private/qdeclarativeanimation_p_p.h"
#include "QtQuick1/private/qdeclarativebehavior_p.h"
#include "QtQuick1/private/qdeclarativebind_p.h"
#include "QtQuick1/private/qdeclarativeconnections_p.h"
#include "QtQuick1/private/qdeclarativesmoothedanimation_p.h"
#include "QtQuick1/private/qdeclarativefontloader_p.h"
#include "QtQuick1/private/qdeclarativelistaccessor_p.h"
//#include "QtQuick1/private/qdeclarativelistmodel_p.h"
#include "QtQuick1/private/qdeclarativeopenmetaobject_p.h"
#include "QtQuick1/private/qdeclarativepackage_p.h"
#include "QtQuick1/private/qdeclarativepixmapcache_p.h"
#include "QtQuick1/private/qdeclarativepropertychanges_p.h"
#include "QtQuick1/private/qdeclarativespringanimation_p.h"
#include "QtQuick1/private/qdeclarativestategroup_p.h"
#include "QtQuick1/private/qdeclarativestateoperations_p.h"
#include "QtQuick1/private/qdeclarativestate_p.h"
#include "QtQuick1/private/qdeclarativestate_p_p.h"
#include "QtQuick1/private/qdeclarativestyledtext_p.h"
#include "QtQuick1/private/qdeclarativesystempalette_p.h"
#include "QtQuick1/private/qdeclarativetimeline_p_p.h"
#include "QtQuick1/private/qdeclarativetimer_p.h"
#include "QtQuick1/private/qdeclarativetransitionmanager_p_p.h"
#include "QtQuick1/private/qdeclarativetransition_p.h"
#include "QtQuick1/private/qdeclarativeapplication_p.h"
#include "QtQuick1/qdeclarativeview.h"
#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtDeclarative/private/qdeclarativetypenotavailable_p.h>
#ifndef QT_NO_XMLPATTERNS
#include "QtQuick1/private/qdeclarativexmllistmodel_p.h"
#endif

QT_BEGIN_NAMESPACE

void QDeclarative1UtilModule::defineModule(QDeclarativeQtQuick1Module::Module module)
{
    qmlRegisterType<QDeclarative1Anchors>();
    qmlRegisterType<QDeclarative1StateOperation>();
    qmlRegisterType<QDeclarative1AnchorSet>();

    if (module == QDeclarativeQtQuick1Module::QtQuick1) {
        qmlRegisterUncreatableType<QDeclarative1Application>("QtQuick",1,1,"Application", QDeclarative1Application::tr("Application is an abstract class"));

        qmlRegisterType<QDeclarative1AnchorAnimation>("QtQuick",1,0,"AnchorAnimation");
        qmlRegisterType<QDeclarative1AnchorChanges>("QtQuick",1,0,"AnchorChanges");
        qmlRegisterType<QDeclarative1Behavior>("QtQuick",1,0,"Behavior");
        qmlRegisterType<QDeclarative1Bind>("QtQuick",1,0,"Binding");
        qmlRegisterType<QDeclarative1ColorAnimation>("QtQuick",1,0,"ColorAnimation");
        qmlRegisterType<QDeclarative1Connections>("QtQuick",1,0,"Connections");
        qmlRegisterType<QDeclarative1SmoothedAnimation>("QtQuick",1,0,"SmoothedAnimation");
        qmlRegisterType<QDeclarative1FontLoader>("QtQuick",1,0,"FontLoader");
    //    qmlRegisterType<QDeclarative1ListElement>("QtQuick",1,0,"ListElement");
        qmlRegisterType<QDeclarative1NumberAnimation>("QtQuick",1,0,"NumberAnimation");
        qmlRegisterType<QDeclarative1Package>("QtQuick",1,0,"Package");
        qmlRegisterType<QDeclarative1ParallelAnimation>("QtQuick",1,0,"ParallelAnimation");
        qmlRegisterType<QDeclarative1ParentAnimation>("QtQuick",1,0,"ParentAnimation");
        qmlRegisterType<QDeclarative1ParentChange>("QtQuick",1,0,"ParentChange");
        qmlRegisterType<QDeclarative1PauseAnimation>("QtQuick",1,0,"PauseAnimation");
        qmlRegisterType<QDeclarative1PropertyAction>("QtQuick",1,0,"PropertyAction");
        qmlRegisterType<QDeclarative1PropertyAnimation>("QtQuick",1,0,"PropertyAnimation");
        qmlRegisterType<QDeclarative1RotationAnimation>("QtQuick",1,0,"RotationAnimation");
        qmlRegisterType<QDeclarative1ScriptAction>("QtQuick",1,0,"ScriptAction");
        qmlRegisterType<QDeclarative1SequentialAnimation>("QtQuick",1,0,"SequentialAnimation");
        qmlRegisterType<QDeclarative1SpringAnimation>("QtQuick",1,0,"SpringAnimation");
        qmlRegisterType<QDeclarative1StateChangeScript>("QtQuick",1,0,"StateChangeScript");
        qmlRegisterType<QDeclarative1StateGroup>("QtQuick",1,0,"StateGroup");
        qmlRegisterType<QDeclarative1State>("QtQuick",1,0,"State");
        qmlRegisterType<QDeclarative1SystemPalette>("QtQuick",1,0,"SystemPalette");
        qmlRegisterType<QDeclarative1Timer>("QtQuick",1,0,"Timer");
        qmlRegisterType<QDeclarative1Transition>("QtQuick",1,0,"Transition");
        qmlRegisterType<QDeclarative1Vector3dAnimation>("QtQuick",1,0,"Vector3dAnimation");
#ifdef QT_NO_XMLPATTERNS
        qmlRegisterTypeNotAvailable("QtQuick",1,0,"XmlListModel",
            qApp->translate("QDeclarative1XmlListModel","Qt was built without support for xmlpatterns"));
        qmlRegisterTypeNotAvailable("QtQuick",1,0,"XmlRole",
            qApp->translate("QDeclarative1XmlListModel","Qt was built without support for xmlpatterns"));
#else
        qmlRegisterType<QDeclarative1XmlListModel>("QtQuick",1,0,"XmlListModel");
        qmlRegisterType<QDeclarative1XmlListModelRole>("QtQuick",1,0,"XmlRole");
#endif


        qmlRegisterUncreatableType<QDeclarative1AbstractAnimation>("QtQuick",1,0,"Animation",QDeclarative1AbstractAnimation::tr("Animation is an abstract class"));

    //    qmlRegisterCustomType<QDeclarative1ListModel>("QtQuick",1,0,"ListModel", new QDeclarative1ListModelParser);
        qmlRegisterCustomType<QDeclarative1PropertyChanges>("QtQuick",1,0,"PropertyChanges", new QDeclarative1PropertyChangesParser);
        qmlRegisterCustomType<QDeclarative1Connections>("QtQuick",1,0,"Connections", new QDeclarative1ConnectionsParser);
    } else if (module == QDeclarativeQtQuick1Module::Qt47) {
        qmlRegisterType<QDeclarative1AnchorAnimation>("Qt",4,7,"AnchorAnimation");
        qmlRegisterType<QDeclarative1AnchorChanges>("Qt",4,7,"AnchorChanges");
        qmlRegisterType<QDeclarative1Behavior>("Qt",4,7,"Behavior");
        qmlRegisterType<QDeclarative1Bind>("Qt",4,7,"Binding");
        qmlRegisterType<QDeclarative1ColorAnimation>("Qt",4,7,"ColorAnimation");
        qmlRegisterType<QDeclarative1Connections>("Qt",4,7,"Connections");
        qmlRegisterType<QDeclarative1SmoothedAnimation>("Qt",4,7,"SmoothedAnimation");
        qmlRegisterType<QDeclarative1FontLoader>("Qt",4,7,"FontLoader");
    //    qmlRegisterType<QDeclarative1ListElement>("Qt",4,7,"ListElement");
        qmlRegisterType<QDeclarative1NumberAnimation>("Qt",4,7,"NumberAnimation");
        qmlRegisterType<QDeclarative1Package>("Qt",4,7,"Package");
        qmlRegisterType<QDeclarative1ParallelAnimation>("Qt",4,7,"ParallelAnimation");
        qmlRegisterType<QDeclarative1ParentAnimation>("Qt",4,7,"ParentAnimation");
        qmlRegisterType<QDeclarative1ParentChange>("Qt",4,7,"ParentChange");
        qmlRegisterType<QDeclarative1PauseAnimation>("Qt",4,7,"PauseAnimation");
        qmlRegisterType<QDeclarative1PropertyAction>("Qt",4,7,"PropertyAction");
        qmlRegisterType<QDeclarative1PropertyAnimation>("Qt",4,7,"PropertyAnimation");
        qmlRegisterType<QDeclarative1RotationAnimation>("Qt",4,7,"RotationAnimation");
        qmlRegisterType<QDeclarative1ScriptAction>("Qt",4,7,"ScriptAction");
        qmlRegisterType<QDeclarative1SequentialAnimation>("Qt",4,7,"SequentialAnimation");
        qmlRegisterType<QDeclarative1SpringAnimation>("Qt",4,7,"SpringAnimation");
        qmlRegisterType<QDeclarative1StateChangeScript>("Qt",4,7,"StateChangeScript");
        qmlRegisterType<QDeclarative1StateGroup>("Qt",4,7,"StateGroup");
        qmlRegisterType<QDeclarative1State>("Qt",4,7,"State");
        qmlRegisterType<QDeclarative1SystemPalette>("Qt",4,7,"SystemPalette");
        qmlRegisterType<QDeclarative1Timer>("Qt",4,7,"Timer");
        qmlRegisterType<QDeclarative1Transition>("Qt",4,7,"Transition");
        qmlRegisterType<QDeclarative1Vector3dAnimation>("Qt",4,7,"Vector3dAnimation");
#ifdef QT_NO_XMLPATTERNS
        qmlRegisterTypeNotAvailable("Qt",4,7,"XmlListModel",
            qApp->translate("QDeclarative1XmlListModel","Qt was built without support for xmlpatterns"));
        qmlRegisterTypeNotAvailable("Qt",4,7,"XmlRole",
            qApp->translate("QDeclarative1XmlListModel","Qt was built without support for xmlpatterns"));
#else
        qmlRegisterType<QDeclarative1XmlListModel>("Qt",4,7,"XmlListModel");
        qmlRegisterType<QDeclarative1XmlListModelRole>("Qt",4,7,"XmlRole");
#endif

        qmlRegisterUncreatableType<QDeclarative1AbstractAnimation>("Qt",4,7,"Animation",QDeclarative1AbstractAnimation::tr("Animation is an abstract class"));

    //    qmlRegisterCustomType<QDeclarative1ListModel>("Qt", 4,7, "ListModel", new QDeclarative1ListModelParser);
        qmlRegisterCustomType<QDeclarative1PropertyChanges>("Qt", 4, 7, "PropertyChanges", new QDeclarative1PropertyChangesParser);
        qmlRegisterCustomType<QDeclarative1Connections>("Qt", 4, 7, "Connections", new QDeclarative1ConnectionsParser);
    }
}

QT_END_NAMESPACE

