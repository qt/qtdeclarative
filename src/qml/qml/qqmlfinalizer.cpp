/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <private/qqmlfinalizer_p.h>

QT_BEGIN_NAMESPACE

QQmlFinalizerHook::~QQmlFinalizerHook() = default;

/*!
    \class QQmlFinalizerHook
    \internal

    QQmlFinalizerHook is an internal interface to run code after the current toplevel
    component has been completed.
    At first, this might look like QQmlParserStaus' componentComplete functionality
    by another name - however there is a difference. To understand it, consider the
    following QML file:

    \qml
    import QtQuick
    Item {
        id: root
        Rectangle {
            id: rect
            Rectangle { id: innerRect }
        }
        property Item it: Item {id: myItem }
    }
    \endqml

    This file will instantiate 4 (sub-)components: One for each of root, rect, innerRect
    and myItem. If the component gets loaded in a synchronous way, each of their
    componentComplete (if existent) would run directly one after another (in a non-specified
    order).

    However, in the case of an asynchronous instantiation, e.g. via a loader, we might interrupt
    the (sub-)component construction partway: There can be a delay between the various
    componentComplete calls, and not all sub-components might have been constructed yet.
    QQmlFinalizerHook::componentFinalized is instead called after all asynchronously instantiated
    (sub-)components have been constructed. Notably, all bindings of those components have also
    been set up.

    \note While the engine does not use qobject_cast, tooling still relies on the interface
    being marked as such. Thus you should always use the Q_INTERFACES macro in classes deriving
    from QQmlFinalizerHook.
*/

/*!
    \fn void QQmlFinalizerHook::componentFinalized()

    The customization point provided by this interface. See the class description for
    why it is useful and how it compares to componentComplete.

    \sa QQmlParserStatus::componentComplete

 */

QT_END_NAMESPACE
