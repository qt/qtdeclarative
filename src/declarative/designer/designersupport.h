/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DESIGNERSUPPORT_H
#define DESIGNERSUPPORT_H

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


#include <QtCore/QtGlobal>
#include <QtCore/QHash>
#include <QtCore/QRectF>

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGItem;
class QSGShaderEffectTexture;
class QImage;
class QTransform;
class QDeclarativeContext;
class QSGView;


class Q_DECLARATIVE_EXPORT DesignerSupport
{
public:
    enum DirtyType {
        TransformOrigin         = 0x00000001,
        Transform               = 0x00000002,
        BasicTransform          = 0x00000004,
        Position                = 0x00000008,
        Size                    = 0x00000010,

        ZValue                  = 0x00000020,
        Content                 = 0x00000040,
        Smooth                  = 0x00000080,
        OpacityValue            = 0x00000100,
        ChildrenChanged         = 0x00000200,
        ChildrenStackingChanged = 0x00000400,
        ParentChanged           = 0x00000800,

        Clip                    = 0x00001000,
        Canvas                  = 0x00002000,

        EffectReference         = 0x00008000,
        Visible                 = 0x00010000,
        HideReference           = 0x00020000,

        TransformUpdateMask     = TransformOrigin | Transform | BasicTransform | Position | Size | Canvas,
        ComplexTransformUpdateMask     = Transform | Canvas,
        ContentUpdateMask       = Size | Content | Smooth | Canvas,
        ChildrenUpdateMask      = ChildrenChanged | ChildrenStackingChanged | EffectReference | Canvas
    };


    DesignerSupport();
    ~DesignerSupport();

    void refFromEffectItem(QSGItem *referencedItem, bool hide = true);
    void derefFromEffectItem(QSGItem *referencedItem, bool unhide = true);

    QImage renderImageForItem(QSGItem *referencedItem, const QRectF &boundingRect, const QSize &imageSize);

    static bool isDirty(QSGItem *referencedItem, DirtyType dirtyType);
    static void resetDirty(QSGItem *referencedItem);

    static QTransform canvasTransform(QSGItem *referencedItem);
    static QTransform parentTransform(QSGItem *referencedItem);

    static bool isAnchoredTo(QSGItem *fromItem, QSGItem *toItem);
    static bool areChildrenAnchoredTo(QSGItem *fromItem, QSGItem *toItem);
    static bool hasAnchor(QSGItem *item, const QString &name);
    static QSGItem *anchorFillTargetItem(QSGItem *item);
    static QSGItem *anchorCenterInTargetItem(QSGItem *item);
    static QPair<QString, QObject*> anchorLineTarget(QSGItem *item, const QString &name, QDeclarativeContext *context);
    static void resetAnchor(QSGItem *item, const QString &name);


    static QList<QObject*> statesForItem(QSGItem *item);

    static bool isComponentComplete(QSGItem *item);

    static int borderWidth(QSGItem *item);

    static void refreshExpressions(QDeclarativeContext *context);

    static void setRootItem(QSGView *view, QSGItem *item);

    static bool isValidWidth(QSGItem *item);
    static bool isValidHeight(QSGItem *item);

    static void updateDirtyNode(QSGItem *item);

private:
    QHash<QSGItem*, QSGShaderEffectTexture*> m_itemTextureHash;
};

QT_END_NAMESPACE

#endif // DESIGNERSUPPORT_H
