/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QSGENGINE_H
#define QSGENGINE_H

#include <QObject>

#include <QtQuick/qsgtexture.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QSGEnginePrivate;

class QQuickCanvas;

class Q_QUICK_EXPORT QSGEngine : public QObject
{
    Q_OBJECT

    Q_DECLARE_PRIVATE(QSGEngine)

public:

    enum TextureOption {
        TextureHasAlphaChannel  = 0x0001,
        TextureHasMipmaps       = 0x0002,
        TextureOwnsGLTexture    = 0x0004
    };
    Q_DECLARE_FLAGS(TextureOptions, TextureOption)

    QSGTexture *createTextureFromImage(const QImage &image) const;
    QSGTexture *createTextureFromId(uint id, const QSize &size, TextureOptions options = TextureOption(0)) const;

    void setClearBeforeRendering(bool enabled);
    bool clearBeforeRendering() const;

    void setClearColor(const QColor &color);
    QColor clearColor() const;

Q_SIGNALS:
    void beforeRendering();
    void afterRendering();

private:
    QSGEngine(QObject *parent = 0);
    ~QSGEngine();

    friend class QSGContext;
    friend class QSGContextPrivate;
    friend class QQuickCanvasPrivate;
    void setCanvas(QQuickCanvas *canvas);

};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSGENGINE_H
