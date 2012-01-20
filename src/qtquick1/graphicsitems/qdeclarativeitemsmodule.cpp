/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QtQuick1/private/qdeclarativeitemsmodule_p.h"

#include <QtWidgets/qaction.h>
#include <QtGui/qvalidator.h>
#include <QtWidgets/qgraphicseffect.h>
#include <QtWidgets/qgraphicsitem.h>

#include "QtQuick1/private/qdeclarativeevents_p_p.h"
#include "QtQuick1/private/qdeclarativescalegrid_p_p.h"
#include "QtQuick1/private/qdeclarativeanimatedimage_p.h"
#include "QtQuick1/private/qdeclarativeborderimage_p.h"
#include "QtQuick1/private/qdeclarativepositioners_p.h"
#include "QtQuick1/private/qdeclarativemousearea_p.h"
#include "QtQuick1/private/qdeclarativeflickable_p.h"
#include "QtQuick1/private/qdeclarativeflickable_p_p.h"
#include "QtQuick1/private/qdeclarativeflipable_p.h"
#include "QtQuick1/private/qdeclarativefocuspanel_p.h"
#include "QtQuick1/private/qdeclarativefocusscope_p.h"
#include "QtQuick1/private/qdeclarativegridview_p.h"
#include "QtQuick1/private/qdeclarativeimage_p.h"
#include "QtQuick1/private/qdeclarativeitem_p.h"
#include "QtQuick1/private/qdeclarativelayoutitem_p.h"
#include "QtQuick1/private/qdeclarativelistview_p.h"
#include "QtQuick1/private/qdeclarativeloader_p.h"
#include "QtQuick1/private/qdeclarativemousearea_p.h"
#include "QtQuick1/private/qdeclarativepath_p.h"
#include "QtQuick1/private/qdeclarativepathview_p.h"
#include "QtQuick1/private/qdeclarativerectangle_p.h"
#include "QtQuick1/private/qdeclarativerepeater_p.h"
#include "QtQuick1/private/qdeclarativetranslate_p.h"
#include "QtQuick1/private/qdeclarativetext_p.h"
#include "QtQuick1/private/qdeclarativetextedit_p.h"
#include "QtQuick1/private/qdeclarativetextinput_p.h"
#include "QtQuick1/private/qdeclarativevisualitemmodel_p.h"
#include "QtQuick1/private/qdeclarativegraphicswidget_p.h"
#include "QtQuick1/private/qdeclarativeanchors_p.h"
#include "QtQuick1/private/qdeclarativepincharea_p.h"
#include "QtQuick1/private/qdeclarativeaccessibleattached_p.h"

static QDeclarativePrivate::AutoParentResult qgraphicsobject_autoParent(QObject *obj, QObject *parent)
{
    QGraphicsObject* gobj = qobject_cast<QGraphicsObject*>(obj);
    if (!gobj)
        return QDeclarativePrivate::IncompatibleObject;

    QGraphicsObject* gparent = qobject_cast<QGraphicsObject*>(parent);
    if (!gparent)
        return QDeclarativePrivate::IncompatibleParent;

    gobj->setParentItem(gparent);
    return QDeclarativePrivate::Parented;
}

void QDeclarative1ItemModule::defineModule(QDeclarativeQtQuick1Module::Module module)
{
    QDeclarativePrivate::RegisterAutoParent autoparent = { 0, &qgraphicsobject_autoParent };
    QDeclarativePrivate::qmlregister(QDeclarativePrivate::AutoParentRegistration, &autoparent);

    qmlRegisterType<QDeclarative1Anchors>();
    qmlRegisterType<QDeclarative1KeyEvent>();
    qmlRegisterType<QDeclarative1MouseEvent>();
    qmlRegisterType<QGraphicsObject>();
    qmlRegisterType<QGraphicsTransform>();
    qmlRegisterType<QDeclarative1PathElement>();
    qmlRegisterType<QDeclarative1Curve>();
    qmlRegisterType<QDeclarative1ScaleGrid>();
#ifndef QT_NO_VALIDATOR
    qmlRegisterType<QValidator>();
#endif
    qmlRegisterType<QDeclarative1VisualModel>();
#ifndef QT_NO_ACTION
    qmlRegisterType<QAction>();
#endif
    qmlRegisterType<QDeclarative1Pen>();
    qmlRegisterType<QDeclarative1FlickableVisibleArea>();
#ifndef QT_NO_GRAPHICSEFFECT
    qmlRegisterType<QGraphicsEffect>();
#endif

    if (module == QDeclarativeQtQuick1Module::QtQuick1) {
#ifdef QT_NO_MOVIE
        qmlRegisterTypeNotAvailable("QtQuick",1,0,"AnimatedImage",
            qApp->translate("QDeclarative1AnimatedImage","Qt was built without support for QMovie"));
#else
        qmlRegisterType<QDeclarative1AnimatedImage>("QtQuick",1,0,"AnimatedImage");
#endif
        qmlRegisterType<QDeclarative1BorderImage>("QtQuick",1,0,"BorderImage");
        qmlRegisterType<QDeclarative1Column>("QtQuick",1,0,"Column");
        qmlRegisterType<QDeclarative1Drag>("QtQuick",1,0,"Drag");
        qmlRegisterType<QDeclarative1Flickable>("QtQuick",1,0,"Flickable");
        qmlRegisterType<QDeclarative1Flipable>("QtQuick",1,0,"Flipable");
        qmlRegisterType<QDeclarative1Flow>("QtQuick",1,0,"Flow");
        qmlRegisterType<QDeclarative1FocusPanel>("QtQuick",1,0,"FocusPanel");
        qmlRegisterType<QDeclarative1FocusScope>("QtQuick",1,0,"FocusScope");
        qmlRegisterType<QDeclarative1Gradient>("QtQuick",1,0,"Gradient");
        qmlRegisterType<QDeclarative1GradientStop>("QtQuick",1,0,"GradientStop");
        qmlRegisterType<QDeclarative1Grid>("QtQuick",1,0,"Grid");
        qmlRegisterType<QDeclarative1GridView>("QtQuick",1,0,"GridView");
        qmlRegisterType<QDeclarative1Image>("QtQuick",1,0,"Image");
        qmlRegisterType<QDeclarativeItem>("QtQuick",1,0,"Item");
        qmlRegisterType<QDeclarative1LayoutItem>("QtQuick",1,0,"LayoutItem");
        qmlRegisterType<QDeclarative1ListView>("QtQuick",1,0,"ListView");
        qmlRegisterType<QDeclarative1Loader>("QtQuick",1,0,"Loader");
        qmlRegisterType<QDeclarative1MouseArea>("QtQuick",1,0,"MouseArea");
        qmlRegisterType<QDeclarative1Path>("QtQuick",1,0,"Path");
        qmlRegisterType<QDeclarative1PathAttribute>("QtQuick",1,0,"PathAttribute");
        qmlRegisterType<QDeclarative1PathCubic>("QtQuick",1,0,"PathCubic");
        qmlRegisterType<QDeclarative1PathLine>("QtQuick",1,0,"PathLine");
        qmlRegisterType<QDeclarative1PathPercent>("QtQuick",1,0,"PathPercent");
        qmlRegisterType<QDeclarative1PathQuad>("QtQuick",1,0,"PathQuad");
        qmlRegisterType<QDeclarative1PathView>("QtQuick",1,0,"PathView");
#ifndef QT_NO_VALIDATOR
        qmlRegisterType<QIntValidator>("QtQuick",1,0,"IntValidator");
        qmlRegisterType<QDoubleValidator>("QtQuick",1,0,"DoubleValidator");
        qmlRegisterType<QRegExpValidator>("QtQuick",1,0,"RegExpValidator");
#endif
        qmlRegisterType<QDeclarative1Rectangle>("QtQuick",1,0,"Rectangle");
        qmlRegisterType<QDeclarative1Repeater>("QtQuick",1,0,"Repeater");
        qmlRegisterType<QGraphicsRotation>("QtQuick",1,0,"Rotation");
        qmlRegisterType<QDeclarative1Row>("QtQuick",1,0,"Row");
        qmlRegisterType<QDeclarative1Translate>("QtQuick",1,0,"Translate");
        qmlRegisterType<QGraphicsScale>("QtQuick",1,0,"Scale");
        qmlRegisterType<QDeclarative1Text>("QtQuick",1,0,"Text");
        qmlRegisterType<QDeclarative1TextEdit>("QtQuick",1,0,"TextEdit");
#ifndef QT_NO_LINEEDIT
        qmlRegisterType<QDeclarative1TextInput>("QtQuick",1,0,"TextInput");
#endif
        qmlRegisterType<QDeclarative1ViewSection>("QtQuick",1,0,"ViewSection");
        qmlRegisterType<QDeclarative1VisualDataModel>("QtQuick",1,0,"VisualDataModel");
        qmlRegisterType<QDeclarative1VisualItemModel>("QtQuick",1,0,"VisualItemModel");

        qmlRegisterType<QGraphicsWidget>("QtQuick",1,0,"QGraphicsWidget");
        qmlRegisterExtendedType<QGraphicsWidget,QDeclarative1GraphicsWidget>("QtQuick",1,0,"QGraphicsWidget");

        qmlRegisterUncreatableType<QDeclarative1KeyNavigationAttached>("QtQuick",1,0,"KeyNavigation",QDeclarative1KeyNavigationAttached::tr("KeyNavigation is only available via attached properties"));
        qmlRegisterUncreatableType<QDeclarative1KeysAttached>("QtQuick",1,0,"Keys",QDeclarative1KeysAttached::tr("Keys is only available via attached properties"));

        // QtQuick 1.1 items
        qmlRegisterType<QDeclarative1PinchArea>("QtQuick",1,1,"PinchArea");
        qmlRegisterType<QDeclarative1Pinch>("QtQuick",1,1,"Pinch");
        qmlRegisterType<QDeclarative1PinchEvent>();
        qmlRegisterType<QDeclarativeItem,1>("QtQuick",1,1,"Item");
        qmlRegisterType<QDeclarative1MouseArea,1>("QtQuick",1,1,"MouseArea");
        qmlRegisterType<QDeclarative1Flickable,1>("QtQuick",1,1,"Flickable");
        qmlRegisterType<QDeclarative1ListView,1>("QtQuick",1,1,"ListView");
        qmlRegisterType<QDeclarative1GridView,1>("QtQuick",1,1,"GridView");
        qmlRegisterType<QDeclarative1Row,1>("QtQuick",1,1,"Row");
        qmlRegisterType<QDeclarative1Grid,1>("QtQuick",1,1,"Grid");
        qmlRegisterType<QDeclarative1Flow,1>("QtQuick",1,1,"Flow");
        qmlRegisterType<QDeclarative1Repeater,1>("QtQuick",1,1,"Repeater");
        qmlRegisterType<QDeclarative1Text,1>("QtQuick",1,1,"Text");
        qmlRegisterType<QDeclarative1TextEdit,1>("QtQuick",1,1,"TextEdit");
#ifndef QT_NO_LINEEDIT
        qmlRegisterType<QDeclarative1TextInput,1>("QtQuick",1,1,"TextInput");
#endif
        qmlRegisterRevision<QDeclarative1ImageBase,1>("QtQuick",1,1);
        qmlRegisterRevision<QDeclarative1ImplicitSizeItem,0>("QtQuick",1,0);
        qmlRegisterRevision<QDeclarative1ImplicitSizeItem,1>("QtQuick",1,1);
        qmlRegisterRevision<QDeclarative1ImplicitSizePaintedItem,0>("QtQuick",1,0);
        qmlRegisterRevision<QDeclarative1ImplicitSizePaintedItem,1>("QtQuick",1,1);
        qmlRegisterUncreatableType<QDeclarative1LayoutMirroringAttached>("QtQuick",1,1,"LayoutMirroring", QDeclarative1LayoutMirroringAttached::tr("LayoutMirroring is only available via attached properties"));
#ifndef QT_NO_ACCESSIBILITY
        qmlRegisterUncreatableType<QDeclarativeAccessibleAttached>("QtQuick",1,0,"Accessible",QDeclarativeAccessibleAttached::tr("Accessible is only available via attached properties"));
        qmlRegisterUncreatableType<QDeclarativeAccessibleAttached>("QtQuick",1,1,"Accessible",QDeclarativeAccessibleAttached::tr("Accessible is only available via attached properties"));
#endif

    } else if (module == QDeclarativeQtQuick1Module::Qt47) {
#ifdef QT_NO_MOVIE
        qmlRegisterTypeNotAvailable("Qt",4,7,"AnimatedImage",
            qApp->translate("QDeclarative1AnimatedImage","Qt was built without support for QMovie"));
#else
        qmlRegisterType<QDeclarative1AnimatedImage>("Qt",4,7,"AnimatedImage");
#endif
        qmlRegisterType<QDeclarative1BorderImage>("Qt",4,7,"BorderImage");
        qmlRegisterType<QDeclarative1Column>("Qt",4,7,"Column");
        qmlRegisterType<QDeclarative1Drag>("Qt",4,7,"Drag");
        qmlRegisterType<QDeclarative1Flickable>("Qt",4,7,"Flickable");
        qmlRegisterType<QDeclarative1Flipable>("Qt",4,7,"Flipable");
        qmlRegisterType<QDeclarative1Flow>("Qt",4,7,"Flow");
        qmlRegisterType<QDeclarative1FocusPanel>("Qt",4,7,"FocusPanel");
        qmlRegisterType<QDeclarative1FocusScope>("Qt",4,7,"FocusScope");
        qmlRegisterType<QDeclarative1Gradient>("Qt",4,7,"Gradient");
        qmlRegisterType<QDeclarative1GradientStop>("Qt",4,7,"GradientStop");
        qmlRegisterType<QDeclarative1Grid>("Qt",4,7,"Grid");
        qmlRegisterType<QDeclarative1GridView>("Qt",4,7,"GridView");
        qmlRegisterType<QDeclarative1Image>("Qt",4,7,"Image");
        qmlRegisterType<QDeclarativeItem>("Qt",4,7,"Item");
        qmlRegisterType<QDeclarative1LayoutItem>("Qt",4,7,"LayoutItem");
        qmlRegisterType<QDeclarative1ListView>("Qt",4,7,"ListView");
        qmlRegisterType<QDeclarative1Loader>("Qt",4,7,"Loader");
        qmlRegisterType<QDeclarative1MouseArea>("Qt",4,7,"MouseArea");
        qmlRegisterType<QDeclarative1Path>("Qt",4,7,"Path");
        qmlRegisterType<QDeclarative1PathAttribute>("Qt",4,7,"PathAttribute");
        qmlRegisterType<QDeclarative1PathCubic>("Qt",4,7,"PathCubic");
        qmlRegisterType<QDeclarative1PathLine>("Qt",4,7,"PathLine");
        qmlRegisterType<QDeclarative1PathPercent>("Qt",4,7,"PathPercent");
        qmlRegisterType<QDeclarative1PathQuad>("Qt",4,7,"PathQuad");
        qmlRegisterType<QDeclarative1PathView>("Qt",4,7,"PathView");
#ifndef QT_NO_VALIDATOR
        qmlRegisterType<QIntValidator>("Qt",4,7,"IntValidator");
        qmlRegisterType<QDoubleValidator>("Qt",4,7,"DoubleValidator");
        qmlRegisterType<QRegExpValidator>("Qt",4,7,"RegExpValidator");
#endif
        qmlRegisterType<QDeclarative1Rectangle>("Qt",4,7,"Rectangle");
        qmlRegisterType<QDeclarative1Repeater>("Qt",4,7,"Repeater");
        qmlRegisterType<QGraphicsRotation>("Qt",4,7,"Rotation");
        qmlRegisterType<QDeclarative1Row>("Qt",4,7,"Row");
        qmlRegisterType<QDeclarative1Translate>("Qt",4,7,"Translate");
        qmlRegisterType<QGraphicsScale>("Qt",4,7,"Scale");
        qmlRegisterType<QDeclarative1Text>("Qt",4,7,"Text");
        qmlRegisterType<QDeclarative1TextEdit>("Qt",4,7,"TextEdit");
#ifndef QT_NO_LINEEDIT
        qmlRegisterType<QDeclarative1TextInput>("Qt",4,7,"TextInput");
#endif
        qmlRegisterType<QDeclarative1ViewSection>("Qt",4,7,"ViewSection");
        qmlRegisterType<QDeclarative1VisualDataModel>("Qt",4,7,"VisualDataModel");
        qmlRegisterType<QDeclarative1VisualItemModel>("Qt",4,7,"VisualItemModel");

        qmlRegisterType<QGraphicsWidget>("Qt",4,7,"QGraphicsWidget");
        qmlRegisterExtendedType<QGraphicsWidget,QDeclarative1GraphicsWidget>("Qt",4,7,"QGraphicsWidget");

        qmlRegisterUncreatableType<QDeclarative1KeyNavigationAttached>("Qt",4,7,"KeyNavigation",QDeclarative1KeyNavigationAttached::tr("KeyNavigation is only available via attached properties"));
        qmlRegisterUncreatableType<QDeclarative1KeysAttached>("Qt",4,7,"Keys",QDeclarative1KeysAttached::tr("Keys is only available via attached properties"));
    }
}



