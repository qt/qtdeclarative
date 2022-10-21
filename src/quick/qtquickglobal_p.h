/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QTQUICKGLOBAL_P_H
#define QTQUICKGLOBAL_P_H

#include <QtQml/private/qtqmlglobal_p.h>
#include <QtGui/private/qtguiglobal_p.h>
#include <QtQuick/private/qtquick-config_p.h>

#include <QtCore/qloggingcategory.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qtquickglobal.h"

#define Q_QUICK_PRIVATE_EXPORT Q_QUICK_EXPORT

void Q_QUICK_PRIVATE_EXPORT qml_register_types_QtQuick();

QT_BEGIN_NAMESPACE

void Q_QUICK_PRIVATE_EXPORT QQuick_initializeModule();

Q_DECLARE_LOGGING_CATEGORY(lcTouch)
Q_DECLARE_LOGGING_CATEGORY(lcMouse)
Q_DECLARE_LOGGING_CATEGORY(lcFocus)
Q_DECLARE_LOGGING_CATEGORY(lcDirty)

/*
    This is needed for QuickTestUtils. Q_AUTOTEST_EXPORT checks QT_BUILDING_QT
    (amongst others) to see if it should export symbols. Until QuickTestUtils
    was introduced, this was enough, as there weren't any intermediate test
    helper libraries that used a Qt library and were in turn used by tests.

    Taking QQuickItemViewPrivate as an example: previously it was using
    Q_AUTOTEST_EXPORT. Since QuickTestUtils is a Qt library (albeit a private
    one), QT_BUILDING_QT was true and so Q_AUTOTEST_EXPORT evaluated to an
    export. However, QQuickItemViewPrivate was already exported by the Quick
    library, so we would get errors like this:

    Qt6Quickd.lib(Qt6Quickd.dll) : error LNK2005: "public: static class
    QQuickItemViewPrivate * __cdecl QQuickItemViewPrivate::get(class QQuickItemView *)"
    (?get@QQuickItemViewPrivate@@SAPEAV1@PEAVQQuickItemView@@@Z) already defined
    in Qt6QuickTestUtilsd.lib(viewtestutils.cpp.obj)

    So, to account for the special case of QuickTestUtils, we need to be more
    specific about which part of Qt we're building; instead of checking if we're
    building any Qt library at all, check if we're building the Quick library,
    and only then export.
*/
#if defined(QT_BUILD_INTERNAL) && defined(QT_BUILD_QUICK_LIB) && defined(QT_SHARED)
#    define Q_QUICK_AUTOTEST_EXPORT Q_DECL_EXPORT
#elif defined(QT_BUILD_INTERNAL) && defined(QT_SHARED)
#    define Q_QUICK_AUTOTEST_EXPORT Q_DECL_IMPORT
#else
#    define Q_QUICK_AUTOTEST_EXPORT
#endif

QT_END_NAMESPACE

#endif // QTQUICKGLOBAL_P_H
