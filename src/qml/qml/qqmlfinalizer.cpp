// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
