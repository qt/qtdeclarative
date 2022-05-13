// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfocusscope_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype FocusScope
    \instantiates QQuickFocusScope
    \inqmlmodule QtQuick
    \ingroup qtquick-input

    \brief Explicitly creates a focus scope.
    \inherits Item

    Focus scopes assist in keyboard focus handling when building reusable QML
    components.  All the details are covered in the
    \l {Keyboard Focus in Qt Quick}{keyboard focus documentation}.

    \sa {Qt Quick Examples - Key Interaction}
*/
QQuickFocusScope::QQuickFocusScope(QQuickItem *parent)
: QQuickItem(parent)
{
    setFlag(ItemIsFocusScope);
}

QT_END_NAMESPACE

#include "moc_qquickfocusscope_p.cpp"
