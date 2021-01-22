/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
