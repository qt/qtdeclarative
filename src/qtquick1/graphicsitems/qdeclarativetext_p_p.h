/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#ifndef QDECLARATIVETEXT_P_H
#define QDECLARATIVETEXT_P_H

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

#include "qdeclarativeitem.h"
#include "private/qdeclarativeimplicitsizeitem_p_p.h"
#include "QtQuick1/private/qdeclarativetextlayout_p.h"

#include <QtDeclarative/qdeclarative.h>

#include <QtGui/qtextlayout.h>

QT_BEGIN_NAMESPACE

class QTextLayout;
class QTextDocumentWithImageResources_1;

class Q_AUTOTEST_EXPORT QDeclarative1TextPrivate : public QDeclarative1ImplicitSizeItemPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1Text)
public:
    QDeclarative1TextPrivate();

    ~QDeclarative1TextPrivate();

    void updateSize();
    void updateLayout();
    bool determineHorizontalAlignment();
    bool setHAlign(QDeclarative1Text::HAlignment, bool forceAlign = false);
    void mirrorChange();
    QTextDocument *textDocument();

    QString text;
    QFont font;
    QFont sourceFont;
    QColor  color;
    QDeclarative1Text::TextStyle style;
    QColor  styleColor;
    QString activeLink;
    QDeclarative1Text::HAlignment hAlign;
    QDeclarative1Text::VAlignment vAlign;
    QDeclarative1Text::TextElideMode elideMode;
    QDeclarative1Text::TextFormat format;
    QDeclarative1Text::WrapMode wrapMode;
    qreal lineHeight;
    QDeclarative1Text::LineHeightMode lineHeightMode;
    int lineCount;
    bool truncated;
    int maximumLineCount;
    int maximumLineCountValid;
    QPointF elidePos;

    static QString elideChar;

    void invalidateImageCache();
    void checkImageCache();
    QPixmap imageCache;

    bool imageCacheDirty:1;
    bool updateOnComponentComplete:1;
    bool richText:1;
    bool singleline:1;
    bool cacheAllTextAsImage:1;
    bool internalWidthUpdate:1;
    bool requireImplicitWidth:1;
    bool hAlignImplicit:1;
    bool rightToLeftText:1;
    bool layoutTextElided:1;

    QRect layedOutTextRect;
    QSize paintedSize;
    qreal naturalWidth;
    virtual qreal implicitWidth() const;
    void ensureDoc();
    QPixmap textDocumentImage(bool drawStyle);
    QTextDocumentWithImageResources_1 *doc;

    QRect setupTextLayout();
    QPixmap textLayoutImage(bool drawStyle);
    void drawTextLayout(QPainter *p, const QPointF &pos, bool drawStyle);
    QDeclarative1TextLayout layout;

    static QPixmap drawOutline(const QPixmap &source, const QPixmap &styleSource);
    static QPixmap drawOutline(const QPixmap &source, const QPixmap &styleSource, int yOffset);

    static inline QDeclarative1TextPrivate *get(QDeclarative1Text *t) {
        return t->d_func();
    }
};

QT_END_NAMESPACE
#endif
