/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKTEXTNODE_P_H
#define QQUICKTEXTNODE_P_H

#include <QtQuick/qsgnode.h>
#include "qquicktext_p.h"
#include <qglyphrun.h>

#include <QtGui/qcolor.h>
#include <QtGui/qtextlayout.h>
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE

class QSGGlyphNode;
class QTextBlock;
class QColor;
class QTextDocument;
class QSGContext;
class QRawFont;
class QSGSimpleRectNode;
class QSGClipNode;
class QSGTexture;

class QQuickTextNode : public QSGTransformNode
{
public:
    enum Decoration {
        NoDecoration = 0x0,
        Underline    = 0x1,
        Overline     = 0x2,
        StrikeOut    = 0x4,
        Background   = 0x8
    };
    Q_DECLARE_FLAGS(Decorations, Decoration)

    QQuickTextNode(QSGContext *, QQuickItem *ownerElement);
    ~QQuickTextNode();

    static bool isComplexRichText(QTextDocument *);

    void deleteContent();
    void addTextLayout(const QPointF &position, QTextLayout *textLayout, const QColor &color = QColor(),
                       QQuickText::TextStyle style = QQuickText::Normal, const QColor &styleColor = QColor(),
                       const QColor &selectionColor = QColor(), const QColor &selectedTextColor = QColor(),
                       int selectionStart = -1, int selectionEnd = -1);
    void addTextDocument(const QPointF &position, QTextDocument *textDocument, const QColor &color = QColor(),
                         QQuickText::TextStyle style = QQuickText::Normal, const QColor &styleColor = QColor(),
                         const QColor &selectionColor = QColor(), const QColor &selectedTextColor = QColor(),
                         int selectionStart = -1, int selectionEnd = -1);

    void setCursor(const QRectF &rect, const QColor &color);
    QSGSimpleRectNode *cursorNode() const { return m_cursorNode; }

    QSGGlyphNode *addGlyphs(const QPointF &position, const QGlyphRun &glyphs, const QColor &color,
                            QQuickText::TextStyle style = QQuickText::Normal, const QColor &styleColor = QColor(),
                            QSGNode *parentNode = 0);
    void addImage(const QRectF &rect, const QImage &image);

private:
    void mergeFormats(QTextLayout *textLayout, QVarLengthArray<QTextLayout::FormatRange> *mergedFormats);

    QSGContext *m_context;
    QSGSimpleRectNode *m_cursorNode;
    QList<QSGTexture *> m_textures;
    QQuickItem *m_ownerElement;
};

QT_END_NAMESPACE

#endif // QQUICKTEXTNODE_P_H
