/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativeutilmodule_p.h"
#include "qdeclarativeanimation_p.h"
#include "qdeclarativeanimation_p_p.h"
#include "qdeclarativebehavior_p.h"
#include "qdeclarativebind_p.h"
#include "qdeclarativeconnections_p.h"
#include "qdeclarativesmoothedanimation_p.h"
#include "qdeclarativefontloader_p.h"
#include "qdeclarativepackage_p.h"
#include "qdeclarativepropertychanges_p.h"
#include "qdeclarativespringanimation_p.h"
#include "qdeclarativestategroup_p.h"
#include "qdeclarativestateoperations_p.h"
#include "qdeclarativestate_p.h"
#include "qdeclarativestate_p_p.h"
#include "qdeclarativesystempalette_p.h"
#include "qdeclarativetimer_p.h"
#include "qdeclarativetransition_p.h"
#include <qdeclarativeinfo.h>
#include <private/qdeclarativetypenotavailable_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/QInputPanel>

void QDeclarativeUtilModule::defineModule()
{
    qmlRegisterUncreatableType<QInputPanel>("QtQuick",2,0,"InputPanel", QInputPanel::tr("InputPanel is an abstract class"));
    qmlRegisterUncreatableType<QDeclarativeAbstractAnimation>("QtQuick",2,0,"Animation",QDeclarativeAbstractAnimation::tr("Animation is an abstract class"));

    qmlRegisterType<QDeclarativeBehavior>("QtQuick",2,0,"Behavior");
    qmlRegisterType<QDeclarativeBind>("QtQuick",2,0,"Binding");
    qmlRegisterType<QDeclarativeColorAnimation>("QtQuick",2,0,"ColorAnimation");
    qmlRegisterType<QDeclarativeConnections>("QtQuick",2,0,"Connections");
    qmlRegisterType<QDeclarativeSmoothedAnimation>("QtQuick",2,0,"SmoothedAnimation");
    qmlRegisterType<QDeclarativeFontLoader>("QtQuick",2,0,"FontLoader");
    qmlRegisterType<QDeclarativeNumberAnimation>("QtQuick",2,0,"NumberAnimation");
    qmlRegisterType<QDeclarativePackage>("QtQuick",2,0,"Package");
    qmlRegisterType<QDeclarativeParallelAnimation>("QtQuick",2,0,"ParallelAnimation");
    qmlRegisterType<QDeclarativePauseAnimation>("QtQuick",2,0,"PauseAnimation");
    qmlRegisterType<QDeclarativePropertyAction>("QtQuick",2,0,"PropertyAction");
    qmlRegisterType<QDeclarativePropertyAnimation>("QtQuick",2,0,"PropertyAnimation");
    qmlRegisterType<QDeclarativeRotationAnimation>("QtQuick",2,0,"RotationAnimation");
    qmlRegisterType<QDeclarativeScriptAction>("QtQuick",2,0,"ScriptAction");
    qmlRegisterType<QDeclarativeSequentialAnimation>("QtQuick",2,0,"SequentialAnimation");
    qmlRegisterType<QDeclarativeSpringAnimation>("QtQuick",2,0,"SpringAnimation");
    qmlRegisterType<QDeclarativeStateChangeScript>("QtQuick",2,0,"StateChangeScript");
    qmlRegisterType<QDeclarativeStateGroup>("QtQuick",2,0,"StateGroup");
    qmlRegisterType<QDeclarativeState>("QtQuick",2,0,"State");
    qmlRegisterType<QDeclarativeSystemPalette>("QtQuick",2,0,"SystemPalette");
    qmlRegisterType<QDeclarativeTimer>("QtQuick",2,0,"Timer");
    qmlRegisterType<QDeclarativeTransition>("QtQuick",2,0,"Transition");
    qmlRegisterType<QDeclarativeVector3dAnimation>("QtQuick",2,0,"Vector3dAnimation");

    qmlRegisterType<QDeclarativeStateOperation>();

    qmlRegisterCustomType<QDeclarativePropertyChanges>("QtQuick",2,0,"PropertyChanges", new QDeclarativePropertyChangesParser);
    qmlRegisterCustomType<QDeclarativeConnections>("QtQuick",2,0,"Connections", new QDeclarativeConnectionsParser);
}
