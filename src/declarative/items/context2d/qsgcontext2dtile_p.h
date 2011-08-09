/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QSGCONTEXT2DTILE_P_H
#define QSGCONTEXT2DTILE_P_H

#include "qsgcontext2d_p.h"
#include <QtOpenGL/QGLFramebufferObject>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGContext2DTexture;
class QSGContext2DCommandBuffer;

class QSGContext2DTile
{
public:
    QSGContext2DTile();
    ~QSGContext2DTile();

    bool dirty() const {return m_dirty;}
    void markDirty(bool dirty) {m_dirty = dirty;}

    QRect rect() const {return m_rect;}

    virtual void setRect(const QRect& r) = 0;
    virtual QPainter* createPainter(bool smooth = false);

protected:
    uint m_dirty : 1;
    QRect m_rect;
    QPaintDevice* m_device;
    QPainter m_painter;
};


class QSGContext2DFBOTile : public QSGContext2DTile
{
public:
    QSGContext2DFBOTile();
    ~QSGContext2DFBOTile();
    virtual void setRect(const QRect& r);
    QGLFramebufferObject* fbo() const {return m_fbo;}
private:
    QGLFramebufferObject *m_fbo;
};

class QSGContext2DImageTile : public QSGContext2DTile
{
public:
    QSGContext2DImageTile();
    ~QSGContext2DImageTile();
    void setRect(const QRect& r);
    const QImage& image() const {return m_image;}
private:
    QImage m_image;
};
QT_END_HEADER

QT_END_NAMESPACE

#endif // QSGCONTEXT2DTILE_P_H
