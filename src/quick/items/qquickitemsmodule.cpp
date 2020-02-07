/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickitemsmodule_p.h"

#include "qquickitem.h"
#include "qquickitem_p.h"
#include "qquickevents_p_p.h"
#include "qquickrectangle_p.h"
#include "qquickfocusscope_p.h"
#include "qquicktext_p.h"
#include "qquicktextinput_p.h"
#include "qquicktextedit_p.h"
#include "qquicktextdocument.h"
#include "qquickimage_p.h"
#include "qquickborderimage_p.h"
#include "qquickscalegrid_p_p.h"
#include "qquickmousearea_p.h"
#include "qquickpincharea_p.h"
#include "qquickflickable_p.h"
#include "qquickflickable_p_p.h"
#if QT_CONFIG(quick_listview)
#include "qquicklistview_p.h"
#endif
#if QT_CONFIG(quick_gridview)
#include "qquickgridview_p.h"
#endif
#if QT_CONFIG(quick_pathview)
#include "qquickpathview_p.h"
#endif
#if QT_CONFIG(quick_tableview)
#include "qquicktableview_p.h"
#endif
#if QT_CONFIG(quick_viewtransitions)
#include "qquickitemviewtransition_p.h"
#endif
#if QT_CONFIG(quick_path)
#include <private/qquickpath_p.h>
#include <private/qquickpathinterpolator_p.h>
#endif
#if QT_CONFIG(quick_positioners)
#include "qquickpositioners_p.h"
#endif
#if QT_CONFIG(quick_repeater)
#include "qquickrepeater_p.h"
#endif
#include "qquickloader_p.h"
#if QT_CONFIG(quick_animatedimage)
#include "qquickanimatedimage_p.h"
#endif
#if QT_CONFIG(quick_flipable)
#include "qquickflipable_p.h"
#endif
#include "qquicktranslate_p.h"
#include "qquickstateoperations_p.h"
#include "qquickitemanimation_p.h"
//#include <private/qquickpincharea_p.h>
#if QT_CONFIG(quick_canvas)
#include <QtQuick/private/qquickcanvasitem_p.h>
#include <QtQuick/private/qquickcontext2d_p.h>
#endif
#include "qquickitemgrabresult.h"
#if QT_CONFIG(quick_sprite)
#include "qquicksprite_p.h"
#include "qquickspritesequence_p.h"
#include "qquickanimatedsprite_p.h"
#endif
#if QT_CONFIG(opengl)
# include "qquickopenglinfo_p.h"
#endif
#include "qquickgraphicsinfo_p.h"
#if QT_CONFIG(quick_shadereffect)
#include <QtQuick/private/qquickshadereffectsource_p.h>
#include "qquickshadereffect_p.h"
#include "qquickshadereffectmesh_p.h"
#endif
#if QT_CONFIG(quick_draganddrop)
#include "qquickdrag_p.h"
#include "qquickdroparea_p.h"
#endif
#include "qquickmultipointtoucharea_p.h"
#include <QtQuick/private/qquickaccessibleattached_p.h>

#include "handlers/qquickdraghandler_p.h"
#include "handlers/qquickhoverhandler_p.h"
#include "handlers/qquickpinchhandler_p.h"
#include "handlers/qquickpointhandler_p.h"
#include "handlers/qquicktaphandler_p.h"
#include "handlers/qquickwheelhandler_p.h"

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcTransient)
QT_END_NAMESPACE

static QQmlPrivate::AutoParentResult qquickitem_autoParent(QObject *obj, QObject *parent)
{
    // When setting a parent (especially during dynamic object creation) in QML,
    // also try to set up the analogous item/window relationship.
    if (QQuickItem *parentItem = qmlobject_cast<QQuickItem *>(parent)) {
        QQuickItem *item = qmlobject_cast<QQuickItem *>(obj);
        if (item) {
            // An Item has another Item
            item->setParentItem(parentItem);
            return QQmlPrivate::Parented;
        } else if (parentItem->window()) {
            QQuickWindow *win = qmlobject_cast<QQuickWindow *>(obj);
            if (win) {
                // A Window inside an Item should be transient for that item's window
                qCDebug(lcTransient) << win << "is transient for" << parentItem->window();
                win->setTransientParent(parentItem->window());
                return QQmlPrivate::Parented;
            }
        } else if (QQuickPointerHandler *handler = qmlobject_cast<QQuickPointerHandler *>(obj)) {
            QQuickItemPrivate::get(parentItem)->addPointerHandler(handler);
            handler->setParent(parent);
            return QQmlPrivate::Parented;
        }
        return QQmlPrivate::IncompatibleObject;
    } else if (QQuickWindow *parentWindow = qmlobject_cast<QQuickWindow *>(parent)) {
        QQuickWindow *win = qmlobject_cast<QQuickWindow *>(obj);
        if (win) {
            // A Window inside a Window should be transient for it
            qCDebug(lcTransient) << win << "is transient for" << parentWindow;
            win->setTransientParent(parentWindow);
            return QQmlPrivate::Parented;
        } else if (QQuickItem *item = qmlobject_cast<QQuickItem *>(obj)) {
            // The parent of an Item inside a Window is actually the implicit content Item
            item->setParentItem(parentWindow->contentItem());
            return QQmlPrivate::Parented;
        } else if (QQuickPointerHandler *handler = qmlobject_cast<QQuickPointerHandler *>(obj)) {
            QQuickItemPrivate::get(parentWindow->contentItem())->addPointerHandler(handler);
            handler->setParent(parentWindow->contentItem());
            return QQmlPrivate::Parented;
        }
        return QQmlPrivate::IncompatibleObject;
    } else if (qmlobject_cast<QQuickItem *>(obj)) {
        return QQmlPrivate::IncompatibleParent;
    }
    return QQmlPrivate::IncompatibleObject;
}

static void qt_quickitems_defineModule()
{
    QQmlPrivate::RegisterAutoParent autoparent = { 0, &qquickitem_autoParent };
    QQmlPrivate::qmlregister(QQmlPrivate::AutoParentRegistration, &autoparent);

    qRegisterMetaType<QQuickAnchorLine>("QQuickAnchorLine");
    qRegisterMetaType<QPointingDeviceUniqueId>("QPointingDeviceUniqueId");
    qRegisterMetaType<QQuickHandlerPoint>();
}

static void initResources()
{
    Q_INIT_RESOURCE(items);
}

QT_BEGIN_NAMESPACE

void QQuickItemsModule::defineModule()
{
    initResources();
    qt_quickitems_defineModule();
}

QT_END_NAMESPACE
