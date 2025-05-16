// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qsgtextureprovider.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSGTextureProvider
    \brief The QSGTextureProvider class encapsulates texture based entities in QML.
    \inmodule QtQuick

    The QSGTextureProvider lives primarily in the scene graph rendering thread.

    \sa {Scene Graph - Two Texture Providers}
 */
/*!
    \fn QSGTexture *QSGTextureProvider::texture() const

    Returns a pointer to the texture object.
 */
/*!
    \fn void QSGTextureProvider::textureChanged()

    This signal is emitted when the texture changes.
 */

QT_END_NAMESPACE

#include "moc_qsgtextureprovider.cpp"
