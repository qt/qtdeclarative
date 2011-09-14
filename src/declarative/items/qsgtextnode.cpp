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

#include "qsgtextnode_p.h"
#include "qsgsimplerectnode.h"
#include <private/qsgadaptationlayer_p.h>
#include <private/qsgdistancefieldglyphcache_p.h>
#include <private/qsgdistancefieldglyphnode_p.h>

#include <private/qsgcontext_p.h>

#include <QtCore/qpoint.h>
#include <qmath.h>
#include <qtextdocument.h>
#include <qtextlayout.h>
#include <qabstracttextdocumentlayout.h>
#include <qxmlstream.h>
#include <qrawfont.h>
#include <private/qdeclarativestyledtext_p.h>
#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#include <private/qrawfont_p.h>
#include <qhash.h>

QT_BEGIN_NAMESPACE

/*!
  Creates an empty QSGTextNode
*/
QSGTextNode::QSGTextNode(QSGContext *context)
    : m_context(context), m_cursorNode(0)
{
#if defined(QML_RUNTIME_TESTING)
    description = QLatin1String("text");
#endif
}

QSGTextNode::~QSGTextNode()
{
}

#if 0
void QSGTextNode::setColor(const QColor &color)
{
    if (m_usePixmapCache) {
        setUpdateFlag(UpdateNodes);
    } else {
        for (QSGNode *childNode = firstChild(); childNode; childNode = childNode->nextSibling()) {
            if (childNode->subType() == GlyphNodeSubType) {
                QSGGlyphNode *glyphNode = static_cast<QSGGlyphNode *>(childNode);
                if (glyphNode->color() == m_color)
                    glyphNode->setColor(color);
            } else if (childNode->subType() == SolidRectNodeSubType) {
                QSGSimpleRectNode *solidRectNode = static_cast<QSGSimpleRectNode *>(childNode);
                if (solidRectNode->color() == m_color)
                    solidRectNode->setColor(color);
            }
        }
    }
    m_color = color;
}

void QSGTextNode::setStyleColor(const QColor &styleColor)
{
    if (m_textStyle != QSGTextNode::NormalTextStyle) {
        if (m_usePixmapCache) {
            setUpdateFlag(UpdateNodes);
        } else {
            for (QSGNode *childNode = firstChild(); childNode; childNode = childNode->nextSibling()) {
                if (childNode->subType() == GlyphNodeSubType) {
                    QSGGlyphNode *glyphNode = static_cast<QSGGlyphNode *>(childNode);
                    if (glyphNode->color() == m_styleColor)
                        glyphNode->setColor(styleColor);
                } else if (childNode->subType() == SolidRectNodeSubType) {
                    QSGSimpleRectNode *solidRectNode = static_cast<QSGSimpleRectNode *>(childNode);
                    if (solidRectNode->color() == m_styleColor)
                        solidRectNode->setColor(styleColor);
                }
            }
        }
    }
    m_styleColor = styleColor;
}
#endif

QSGGlyphNode *QSGTextNode::addGlyphs(const QPointF &position, const QGlyphRun &glyphs, const QColor &color,
                                     QSGText::TextStyle style, const QColor &styleColor,
                                     QSGNode *parentNode)
{
    QSGGlyphNode *node = m_context->createGlyphNode();
    node->setGlyphs(position + QPointF(0, glyphs.rawFont().ascent()), glyphs);
    node->setStyle(style);
    node->setStyleColor(styleColor);
    node->setColor(color);
    node->update();

    /* We flag the geometry as static, but we never call markVertexDataDirty
       or markIndexDataDirty on them. This is because all text nodes are
       discarded when a change occurs. If we start appending/removing from
       existing geometry, then we also need to start marking the geometry as
       dirty.
     */
    node->geometry()->setIndexDataPattern(QSGGeometry::StaticPattern);
    node->geometry()->setVertexDataPattern(QSGGeometry::StaticPattern);

    if (parentNode == 0)
        parentNode = this;
    parentNode->appendChildNode(node);

    return node;
}

void QSGTextNode::setCursor(const QRectF &rect, const QColor &color)
{
    if (m_cursorNode != 0)
        delete m_cursorNode;

    m_cursorNode = new QSGSimpleRectNode(rect, color);
    appendChildNode(m_cursorNode);
}

namespace {

    struct BinaryTreeNode {
        enum SelectionState {
            Unselected,
            Selected
        };

        BinaryTreeNode()
            : selectionState(Unselected)
            , clipNode(0)
            , decorations(QSGTextNode::NoDecoration)
            , leftChildIndex(-1)
            , rightChildIndex(-1)
        {

        }

        BinaryTreeNode(const QGlyphRun &g, SelectionState selState, const QRectF &brect,
                       const QSGTextNode::Decorations &decs,
                       const QColor &c, const QColor &bc,
                       const QPointF &pos)
            : glyphRun(g)
            , boundingRect(brect)
            , selectionState(selState)
            , clipNode(0)
            , decorations(decs)
            , color(c)
            , backgroundColor(bc)
            , position(pos)
            , leftChildIndex(-1)
            , rightChildIndex(-1)
        {
        }

        QGlyphRun glyphRun;
        QRectF boundingRect;
        SelectionState selectionState;
        QSGClipNode *clipNode;
        QSGTextNode::Decorations decorations;
        QColor color;
        QColor backgroundColor;
        QPointF position;

        int leftChildIndex;
        int rightChildIndex;

        static void insert(QVarLengthArray<BinaryTreeNode> *binaryTree,
                           const QGlyphRun &glyphRun,
                           SelectionState selectionState,
                           const QColor &textColor,
                           const QColor &backgroundColor,
                           const QPointF &position)
        {
            int newIndex = binaryTree->size();
            QRectF searchRect = glyphRun.boundingRect();

            if (qFuzzyIsNull(searchRect.width()) || qFuzzyIsNull(searchRect.height()))
                return;

            QSGTextNode::Decorations decorations = QSGTextNode::NoDecoration;
            decorations |= (glyphRun.underline() ? QSGTextNode::Underline : QSGTextNode::NoDecoration);
            decorations |= (glyphRun.overline()  ? QSGTextNode::Overline  : QSGTextNode::NoDecoration);
            decorations |= (glyphRun.strikeOut() ? QSGTextNode::StrikeOut : QSGTextNode::NoDecoration);
            decorations |= (backgroundColor.isValid() ? QSGTextNode::Background : QSGTextNode::NoDecoration);

            binaryTree->append(BinaryTreeNode(glyphRun, selectionState, searchRect, decorations,
                                              textColor, backgroundColor, position));
            if (newIndex == 0)
                return;

            int searchIndex = 0;
            forever {
                BinaryTreeNode *node = binaryTree->data() + searchIndex;
                if (searchRect.left() < node->boundingRect.left()) {
                    if (node->leftChildIndex < 0) {
                        node->leftChildIndex = newIndex;
                        break;
                    } else {
                        searchIndex = node->leftChildIndex;
                    }
                } else {
                    if (node->rightChildIndex < 0) {
                        node->rightChildIndex = newIndex;
                        break;
                    } else {
                        searchIndex = node->rightChildIndex;
                    }
                }
            }
        }

        static void inOrder(const QVarLengthArray<BinaryTreeNode> &binaryTree,
                            QVarLengthArray<int> *sortedIndexes,
                            int currentIndex = 0)
        {
            Q_ASSERT(currentIndex < binaryTree.size());

            const BinaryTreeNode *node = binaryTree.data() + currentIndex;
            if (node->leftChildIndex >= 0)
                inOrder(binaryTree, sortedIndexes, node->leftChildIndex);

            sortedIndexes->append(currentIndex);

            if (node->rightChildIndex >= 0)
                inOrder(binaryTree, sortedIndexes, node->rightChildIndex);
        }
    };

    // Engine that takes glyph runs as input, and produces a set of glyph nodes, clip nodes,
    // and rectangle nodes to represent the text, decorations and selection. Will try to minimize
    // number of nodes, and join decorations in neighbouring items
    class SelectionEngine
    {
    public:
        SelectionEngine() : m_hasSelection(false) {}

        QTextLine currentLine() const { return m_currentLine; }

        void setCurrentLine(const QTextLine &currentLine)
        {
            if (m_currentLine.isValid())
                processCurrentLine();

            m_currentLine = currentLine;
        }

        void addSelectedGlyphs(const QGlyphRun &glyphRun);
        void addUnselectedGlyphs(const QGlyphRun &glyphRun);
        void addGlyphsInRange(int rangeStart, int rangeEnd,
                              const QColor &color, const QColor &backgroundColor,
                              int selectionStart, int selectionEnd);
        void addGlyphsForRanges(const QVarLengthArray<QTextLayout::FormatRange> &ranges,
                                int start, int end,
                                int selectionStart, int selectionEnd);

        void addToSceneGraph(QSGTextNode *parent,
                             QSGText::TextStyle style = QSGText::Normal,
                             const QColor &styleColor = QColor());

        void setSelectionColor(const QColor &selectionColor)
        {
            m_selectionColor = selectionColor;
        }

        void setSelectedTextColor(const QColor &selectedTextColor)
        {
            m_selectedTextColor = selectedTextColor;
        }

        void setTextColor(const QColor &textColor)
        {
            m_textColor = textColor;
        }

        void setPosition(const QPointF &position)
        {
            m_position = position;
        }

    private:
        struct TextDecoration
        {
            TextDecoration() : selectionState(BinaryTreeNode::Unselected) {}
            TextDecoration(const BinaryTreeNode::SelectionState &s,
                           const QRectF &r,
                           const QColor &c)
                : selectionState(s)
                , rect(r)
                , color(c)
            {
            }

            BinaryTreeNode::SelectionState selectionState;
            QRectF rect;
            QColor color;
        };

        void processCurrentLine();
        void addTextDecorations(const QVarLengthArray<TextDecoration> &textDecorations,
                                qreal offset, qreal thickness);

        QColor m_selectionColor;
        QColor m_textColor;
        QColor m_backgroundColor;
        QColor m_selectedTextColor;
        QPointF m_position;

        QTextLine m_currentLine;
        bool m_hasSelection;

        QList<QRectF> m_selectionRects;
        QVarLengthArray<BinaryTreeNode> m_currentLineTree;

        QList<TextDecoration> m_lines;
        QVector<BinaryTreeNode> m_processedNodes;
    };

    void SelectionEngine::addTextDecorations(const QVarLengthArray<TextDecoration> &textDecorations,
                                             qreal offset, qreal thickness)
    {
        for (int i=0; i<textDecorations.size(); ++i) {
            TextDecoration textDecoration = textDecorations.at(i);

            {
                QRectF &rect = textDecoration.rect;
                rect.setY(qRound(rect.y() + m_currentLine.ascent() + offset));
                rect.setHeight(thickness);
            }

            m_lines.append(textDecoration);
        }
    }

    void SelectionEngine::processCurrentLine()
    {
        // No glyphs, do nothing
        if (m_currentLineTree.isEmpty())
            return;

        // 1. Go through current line and get correct decoration position for each node based on
        // neighbouring decorations. Add decoration to global list
        // 2. Create clip nodes for all selected text. Try to merge as many as possible within
        // the line.
        // 3. Add QRects to a list of selection rects.
        // 4. Add all nodes to a global processed list
        QVarLengthArray<int> sortedIndexes; // Indexes in tree sorted by x position
        BinaryTreeNode::inOrder(m_currentLineTree, &sortedIndexes);

        Q_ASSERT(sortedIndexes.size() == m_currentLineTree.size());

        BinaryTreeNode::SelectionState currentSelectionState = BinaryTreeNode::Unselected;
        QRectF currentRect;

        QSGTextNode::Decorations currentDecorations = QSGTextNode::NoDecoration;
        qreal underlineOffset = 0.0;
        qreal underlineThickness = 0.0;

        qreal overlineOffset = 0.0;
        qreal overlineThickness = 0.0;

        qreal strikeOutOffset = 0.0;
        qreal strikeOutThickness = 0.0;

        QRectF decorationRect = currentRect;

        QColor lastColor;
        QColor lastBackgroundColor;

        QVarLengthArray<TextDecoration> pendingUnderlines;
        QVarLengthArray<TextDecoration> pendingOverlines;
        QVarLengthArray<TextDecoration> pendingStrikeOuts;
        QVarLengthArray<TextDecoration> pendingBackgrounds;
        if (!sortedIndexes.isEmpty()) {
            QSGClipNode *currentClipNode = m_hasSelection ? new QSGClipNode : 0;
            bool currentClipNodeUsed = false;
            for (int i=0; i<=sortedIndexes.size(); ++i) {
                BinaryTreeNode *node = 0;
                if (i < sortedIndexes.size()) {
                    int sortedIndex = sortedIndexes.at(i);
                    Q_ASSERT(sortedIndex < m_currentLineTree.size());

                    node = m_currentLineTree.data() + sortedIndex;
                }

                if (i == 0)
                    currentSelectionState = node->selectionState;

                // Update decorations
                if (currentDecorations != QSGTextNode::NoDecoration) {
                    decorationRect.setY(m_position.y() + m_currentLine.y());
                    decorationRect.setHeight(m_currentLine.height());

                    if (node != 0)
                        decorationRect.setRight(node->boundingRect.left());

                    TextDecoration textDecoration(currentSelectionState, decorationRect, lastColor);
                    if (currentDecorations & QSGTextNode::Underline)
                        pendingUnderlines.append(textDecoration);

                    if (currentDecorations & QSGTextNode::Overline)
                        pendingOverlines.append(textDecoration);

                    if (currentDecorations & QSGTextNode::StrikeOut)
                        pendingStrikeOuts.append(textDecoration);

                    if (currentDecorations & QSGTextNode::Background) {
                        pendingBackgrounds.append(TextDecoration(BinaryTreeNode::Unselected,
                                                                 decorationRect,
                                                                 lastBackgroundColor));
                    }
                }

                // If we've reached an unselected node from a selected node, we add the
                // selection rect to the graph, and we add decoration every time the
                // selection state changes, because that means the text color changes
                if (node == 0 || node->selectionState != currentSelectionState) {
                    if (node != 0)
                        currentRect.setRight(node->boundingRect.left());
                    currentRect.setY(m_position.y() + m_currentLine.y());
                    currentRect.setHeight(m_currentLine.height());

                    // Draw selection all the way up to the left edge of the unselected item
                    if (currentSelectionState == BinaryTreeNode::Selected)
                        m_selectionRects.append(currentRect);

                    if (currentClipNode != 0) {
                        if (!currentClipNodeUsed) {
                            delete currentClipNode;
                        } else {
                            currentClipNode->setIsRectangular(true);
                            currentClipNode->setClipRect(currentRect);
                        }
                    }

                    if (node != 0 && m_hasSelection)
                        currentClipNode = new QSGClipNode;
                    else
                        currentClipNode = 0;
                    currentClipNodeUsed = false;

                    if (node != 0) {
                        currentSelectionState = node->selectionState;
                        currentRect = node->boundingRect;

                        // Make sure currentRect is valid, otherwise the unite won't work
                        if (currentRect.isNull())
                            currentRect.setSize(QSizeF(1, 1));
                    }
                } else {
                    if (currentRect.isNull())
                        currentRect = node->boundingRect;
                    else
                        currentRect = currentRect.united(node->boundingRect);
                }

                if (node != 0) {
                    node->clipNode = currentClipNode;
                    currentClipNodeUsed = true;

                    decorationRect = node->boundingRect;

                    if (!pendingBackgrounds.isEmpty()) {
                        addTextDecorations(pendingBackgrounds, -m_currentLine.ascent(),
                                           m_currentLine.height());

                        pendingBackgrounds.clear();
                    }

                    // If previous item(s) had underline and current does not, then we add the
                    // pending lines to the lists and likewise for overlines and strikeouts
                    if (!pendingUnderlines.isEmpty()
                      && !(node->decorations & QSGTextNode::Underline)) {
                        addTextDecorations(pendingUnderlines, underlineOffset, underlineThickness);

                        pendingUnderlines.clear();

                        underlineOffset = 0.0;
                        underlineThickness = 0.0;
                    }

                    // ### Add pending when overlineOffset/thickness changes to minimize number of
                    // nodes
                    if (!pendingOverlines.isEmpty()) {
                        addTextDecorations(pendingOverlines, overlineOffset, overlineThickness);

                        pendingOverlines.clear();

                        overlineOffset = 0.0;
                        overlineThickness = 0.0;
                    }

                    // ### Add pending when overlineOffset/thickness changes to minimize number of
                    // nodes
                    if (!pendingStrikeOuts.isEmpty()) {
                        addTextDecorations(pendingStrikeOuts, strikeOutOffset, strikeOutThickness);

                        pendingStrikeOuts.clear();

                        strikeOutOffset = 0.0;
                        strikeOutThickness = 0.0;
                    }

                    // Merge current values with previous. Prefer greatest thickness
                    QRawFont rawFont = node->glyphRun.rawFont();
                    if (node->decorations & QSGTextNode::Underline) {
                        if (rawFont.lineThickness() > underlineThickness) {
                            underlineThickness = rawFont.lineThickness();
                            underlineOffset = rawFont.underlinePosition();
                        }
                    }

                    if (node->decorations & QSGTextNode::Overline) {
                        overlineOffset = -rawFont.ascent();
                        overlineThickness = rawFont.lineThickness();
                    }

                    if (node->decorations & QSGTextNode::StrikeOut) {
                        strikeOutThickness = rawFont.lineThickness();
                        strikeOutOffset = rawFont.ascent() / -3.0;
                    }

                    currentDecorations = node->decorations;
                    lastColor = node->color;
                    lastBackgroundColor = node->backgroundColor;
                    m_processedNodes.append(*node);
                }
            }

            // If there are pending decorations, we need to add them
            if (!pendingBackgrounds.isEmpty()) {
                addTextDecorations(pendingBackgrounds, -m_currentLine.ascent(),
                                   m_currentLine.height());
            }

            if (!pendingUnderlines.isEmpty())
                addTextDecorations(pendingUnderlines, underlineOffset, underlineThickness);

            if (!pendingOverlines.isEmpty())
                addTextDecorations(pendingOverlines, overlineOffset, overlineThickness);

            if (!pendingStrikeOuts.isEmpty())
                addTextDecorations(pendingStrikeOuts, strikeOutOffset, strikeOutThickness);
        }

        m_currentLineTree.clear();
        m_currentLine = QTextLine();
        m_hasSelection = false;
    }

    void SelectionEngine::addUnselectedGlyphs(const QGlyphRun &glyphRun)
    {
        BinaryTreeNode::insert(&m_currentLineTree, glyphRun, BinaryTreeNode::Unselected,
                               m_textColor, m_backgroundColor, m_position);
    }

    void SelectionEngine::addSelectedGlyphs(const QGlyphRun &glyphRun)
    {
        int currentSize = m_currentLineTree.size();
        BinaryTreeNode::insert(&m_currentLineTree, glyphRun, BinaryTreeNode::Selected,
                               m_textColor, m_backgroundColor, m_position);
        m_hasSelection = m_hasSelection || m_currentLineTree.size() > currentSize;
    }

    void SelectionEngine::addGlyphsForRanges(const QVarLengthArray<QTextLayout::FormatRange> &ranges,
                                             int start, int end,
                                             int selectionStart, int selectionEnd)
    {
        int currentPosition = start;
        int remainingLength = end - start;
        for (int j=0; j<ranges.size(); ++j) {
            const QTextLayout::FormatRange &range = ranges.at(j);
            if (range.start + range.length >= currentPosition
                && range.start < currentPosition + remainingLength) {

                if (range.start > currentPosition) {
                    addGlyphsInRange(currentPosition, range.start - currentPosition,
                                     QColor(), QColor(), selectionStart, selectionEnd);
                }

                int rangeEnd = qMin(range.start + range.length, currentPosition + remainingLength);
                QColor rangeColor = range.format.hasProperty(QTextFormat::ForegroundBrush)
                        ? range.format.foreground().color()
                        : QColor();
                QColor rangeBackgroundColor = range.format.hasProperty(QTextFormat::BackgroundBrush)
                        ? range.format.background().color()
                        : QColor();

                addGlyphsInRange(range.start, rangeEnd - range.start,
                                 rangeColor, rangeBackgroundColor,
                                 selectionStart, selectionEnd);

                currentPosition = range.start + range.length;
                remainingLength = end - currentPosition;

            } else if (range.start > currentPosition + remainingLength || remainingLength <= 0) {
                break;
            }
        }

        if (remainingLength > 0) {
            addGlyphsInRange(currentPosition, remainingLength, QColor(), QColor(),
                             selectionStart, selectionEnd);
        }

    }

    void SelectionEngine::addGlyphsInRange(int rangeStart, int rangeLength,
                                           const QColor &color, const QColor &backgroundColor,
                                           int selectionStart, int selectionEnd)
    {
        QColor oldColor;
        if (color.isValid()) {
            oldColor = m_textColor;
            m_textColor = color;
        }

        QColor oldBackgroundColor = m_backgroundColor;
        if (backgroundColor.isValid()) {
            oldBackgroundColor = m_backgroundColor;
            m_backgroundColor = backgroundColor;
        }

        bool hasSelection = selectionStart >= 0
                         && selectionEnd >= 0
                         && selectionStart != selectionEnd;

        QTextLine &line = m_currentLine;
        int rangeEnd = rangeStart + rangeLength;
        if (!hasSelection || (selectionStart > rangeEnd || selectionEnd < rangeStart)) {
            QList<QGlyphRun> glyphRuns = line.glyphRuns(rangeStart, rangeLength);
            for (int j=0; j<glyphRuns.size(); ++j)
                addUnselectedGlyphs(glyphRuns.at(j));
        } else {
            if (rangeStart < selectionStart) {
                QList<QGlyphRun> glyphRuns = line.glyphRuns(rangeStart,
                                                            qMin(selectionStart - rangeStart,
                                                                 rangeLength));

                for (int j=0; j<glyphRuns.size(); ++j)
                    addUnselectedGlyphs(glyphRuns.at(j));
            }

            if (rangeEnd >= selectionStart && selectionStart >= rangeStart) {
                QList<QGlyphRun> glyphRuns = line.glyphRuns(selectionStart, selectionEnd - selectionStart + 1);

                for (int j=0; j<glyphRuns.size(); ++j)
                    addSelectedGlyphs(glyphRuns.at(j));
            }

            if (selectionEnd >= rangeStart && selectionEnd < rangeEnd) {
                QList<QGlyphRun> glyphRuns = line.glyphRuns(selectionEnd + 1, rangeEnd - selectionEnd);
                for (int j=0; j<glyphRuns.size(); ++j)
                    addUnselectedGlyphs(glyphRuns.at(j));
            }
        }

        if (backgroundColor.isValid())
            m_backgroundColor = oldBackgroundColor;

        if (oldColor.isValid())
            m_textColor = oldColor;
    }

    void SelectionEngine::addToSceneGraph(QSGTextNode *parentNode,
                                          QSGText::TextStyle style,
                                          const QColor &styleColor)
    {
        if (m_currentLine.isValid())
            processCurrentLine();

        // First, prepend all selection rectangles to the tree
        for (int i=0; i<m_selectionRects.size(); ++i) {
            const QRectF &rect = m_selectionRects.at(i);

            parentNode->appendChildNode(new QSGSimpleRectNode(rect, m_selectionColor));
        }

        // Finally, add decorations for each node to the tree.
        for (int i=0; i<m_lines.size(); ++i) {
            const TextDecoration &textDecoration = m_lines.at(i);

            QColor color = textDecoration.selectionState == BinaryTreeNode::Selected
                    ? m_selectedTextColor
                    : textDecoration.color;

            parentNode->appendChildNode(new QSGSimpleRectNode(textDecoration.rect, color));
        }

        // Then, go through all the nodes for all lines and combine all QGlyphRuns with a common
        // font, selection state and clip node.
        typedef QPair<QFontEngine *, QPair<QSGClipNode *, QPair<QRgb, int> > > KeyType;
        QHash<KeyType, BinaryTreeNode *> map;
        for (int i=0; i<m_processedNodes.size(); ++i) {
            BinaryTreeNode *node = m_processedNodes.data() + i;

            QGlyphRun glyphRun = node->glyphRun;
            QRawFont rawFont = glyphRun.rawFont();
            QRawFontPrivate *rawFontD = QRawFontPrivate::get(rawFont);

            QFontEngine *fontEngine = rawFontD->fontEngine;

            KeyType key(qMakePair(fontEngine,
                                  qMakePair(node->clipNode,
                                            qMakePair(node->color.rgba(), int(node->selectionState)))));

            BinaryTreeNode *otherNode = map.value(key, 0);
            if (otherNode != 0) {
                QGlyphRun &otherGlyphRun = otherNode->glyphRun;

                QVector<quint32> otherGlyphIndexes = otherGlyphRun.glyphIndexes();
                QVector<QPointF> otherGlyphPositions = otherGlyphRun.positions();

                otherGlyphIndexes += glyphRun.glyphIndexes();

                QVector<QPointF> glyphPositions = glyphRun.positions();
                for (int j=0; j<glyphPositions.size(); ++j) {
                    otherGlyphPositions += glyphPositions.at(j) + (node->position - otherNode->position);
                }

                otherGlyphRun.setGlyphIndexes(otherGlyphIndexes);
                otherGlyphRun.setPositions(otherGlyphPositions);

            } else {
                map.insert(key, node);
            }
        }

        // ...and add clip nodes and glyphs to tree.
        QHash<KeyType, BinaryTreeNode *>::const_iterator it = map.constBegin();
        while (it != map.constEnd()) {

            BinaryTreeNode *node = it.value();

            QSGClipNode *clipNode = node->clipNode;
            if (clipNode != 0 && clipNode->parent() == 0 )
                parentNode->appendChildNode(clipNode);

            QColor color = node->selectionState == BinaryTreeNode::Selected
                    ? m_selectedTextColor
                    : node->color;

            parentNode->addGlyphs(node->position, node->glyphRun, color, style, styleColor, clipNode);

            ++it;
        }
    }
}

void QSGTextNode::addTextDocument(const QPointF &, QTextDocument *textDocument,
                                  const QColor &textColor,
                                  QSGText::TextStyle style, const QColor &styleColor,
                                  const QColor &selectionColor, const QColor &selectedTextColor,
                                  int selectionStart, int selectionEnd)
{
    QTextFrame *textFrame = textDocument->rootFrame();
    QPointF position = textDocument->documentLayout()->frameBoundingRect(textFrame).topLeft();

    SelectionEngine engine;
    engine.setTextColor(textColor);
    engine.setSelectedTextColor(selectedTextColor);
    engine.setSelectionColor(selectionColor);

    QTextFrame::iterator it = textFrame->begin();
    while (!it.atEnd()) {
        Q_ASSERT(!engine.currentLine().isValid());

        QTextBlock block = it.currentBlock();
        int preeditLength = block.isValid() ? block.layout()->preeditAreaText().length() : 0;
        int preeditPosition = block.isValid() ? block.layout()->preeditAreaPosition() : -1;

        QTextLayout *textLayout = block.layout();
        QList<QTextLayout::FormatRange> additionalFormats;
        if (textLayout != 0)
            additionalFormats = textLayout->additionalFormats();
        QVarLengthArray<QTextLayout::FormatRange> colorChanges;
        for (int i=0; i<additionalFormats.size(); ++i) {
            QTextLayout::FormatRange additionalFormat = additionalFormats.at(i);
            if (additionalFormat.format.hasProperty(QTextFormat::ForegroundBrush)
             || additionalFormat.format.hasProperty(QTextFormat::BackgroundBrush)) {
                // Merge overlapping formats
                if (!colorChanges.isEmpty()) {
                    QTextLayout::FormatRange *lastFormat = colorChanges.data() + colorChanges.size() - 1;

                    if (additionalFormat.start < lastFormat->start + lastFormat->length) {
                        QTextLayout::FormatRange *mergedRange = 0;

                        int length = additionalFormat.length;
                        if (additionalFormat.start > lastFormat->start) {
                            lastFormat->length = additionalFormat.start - lastFormat->start;
                            length -= lastFormat->length;

                            colorChanges.append(QTextLayout::FormatRange());
                            mergedRange = colorChanges.data() + colorChanges.size() - 1;
                            lastFormat = colorChanges.data() + colorChanges.size() - 2;
                        } else {
                            mergedRange = lastFormat;
                        }

                        mergedRange->format = lastFormat->format;
                        mergedRange->format.merge(additionalFormat.format);
                        mergedRange->start = additionalFormat.start;

                        int end = qMin(additionalFormat.start + additionalFormat.length,
                                       lastFormat->start + lastFormat->length);

                        mergedRange->length = end - mergedRange->start;
                        length -= mergedRange->length;

                        additionalFormat.start = end;
                        additionalFormat.length = length;
                    }
                }

                if (additionalFormat.length > 0)
                    colorChanges.append(additionalFormat);
            }
        }

        QTextBlock::iterator blockIterator = block.begin();
        int textPos = block.position();
        while (!blockIterator.atEnd()) {
            QTextFragment fragment = blockIterator.fragment();
            if (fragment.text().isEmpty())
                continue;

            QPointF blockPosition = textDocument->documentLayout()->blockBoundingRect(block).topLeft();
            engine.setPosition(position + blockPosition);

            QTextCharFormat charFormat = fragment.charFormat();
            if (!textColor.isValid())
                engine.setTextColor(charFormat.foreground().color());

            int fragmentEnd = textPos + fragment.length();
            if (preeditPosition >= 0
             && preeditPosition >= textPos
             && preeditPosition < fragmentEnd) {
                fragmentEnd += preeditLength;
            }

            while (textPos < fragmentEnd) {
                int blockRelativePosition = textPos - block.position();
                QTextLine line = block.layout()->lineForTextPosition(blockRelativePosition);
                Q_ASSERT(line.textLength() > 0);
                if (!engine.currentLine().isValid() || line.lineNumber() != engine.currentLine().lineNumber())
                    engine.setCurrentLine(line);

                int lineEnd = line.textStart() + block.position() + line.textLength();

                int len = qMin(lineEnd - textPos, fragmentEnd - textPos);
                Q_ASSERT(len > 0);

                int currentStepEnd = textPos + len;

                engine.addGlyphsForRanges(colorChanges, textPos, currentStepEnd,
                                          selectionStart, selectionEnd);

                textPos = currentStepEnd;
            }

            ++blockIterator;
        }

        engine.setCurrentLine(QTextLine()); // Reset current line because the text layout changed
        ++it;
    }

    engine.addToSceneGraph(this, style, styleColor);
}

void QSGTextNode::addTextLayout(const QPointF &position, QTextLayout *textLayout, const QColor &color,
                                QSGText::TextStyle style, const QColor &styleColor,
                                const QColor &selectionColor, const QColor &selectedTextColor,
                                int selectionStart, int selectionEnd)
{
    SelectionEngine engine;
    engine.setTextColor(color);
    engine.setSelectedTextColor(selectedTextColor);
    engine.setSelectionColor(selectionColor);
    engine.setPosition(position);

    QList<QTextLayout::FormatRange> additionalFormats = textLayout->additionalFormats();
    QVarLengthArray<QTextLayout::FormatRange> colorChanges;
    for (int i=0; i<additionalFormats.size(); ++i) {
        if (additionalFormats.at(i).format.hasProperty(QTextFormat::ForegroundBrush))
            colorChanges.append(additionalFormats.at(i));
    }

    for (int i=0; i<textLayout->lineCount(); ++i) {
        QTextLine line = textLayout->lineAt(i);

        engine.setCurrentLine(line);
        engine.addGlyphsForRanges(colorChanges,
                                  line.textStart(), line.textStart() + line.textLength(),
                                  selectionStart, selectionEnd);
    }

    engine.addToSceneGraph(this, style, styleColor);
}


/*!
  Returns true if \a text contains any HTML tags, attributes or CSS properties which are unrelated
   to text, fonts or text layout. Otherwise the function returns false. If the return value is
  false, \a text is considered to be easily representable in the scenegraph. If it returns true,
  then the text should be prerendered into a pixmap before it's displayed on screen.
*/
bool QSGTextNode::isComplexRichText(QTextDocument *doc)
{
    if (doc == 0)
        return false;

    static QSet<QString> supportedTags;
    if (supportedTags.isEmpty()) {
        supportedTags.insert(QLatin1String("i"));
        supportedTags.insert(QLatin1String("b"));
        supportedTags.insert(QLatin1String("u"));
        supportedTags.insert(QLatin1String("div"));
        supportedTags.insert(QLatin1String("big"));
        supportedTags.insert(QLatin1String("blockquote"));
        supportedTags.insert(QLatin1String("body"));
        supportedTags.insert(QLatin1String("br"));
        supportedTags.insert(QLatin1String("center"));
        supportedTags.insert(QLatin1String("cite"));
        supportedTags.insert(QLatin1String("code"));
        supportedTags.insert(QLatin1String("tt"));
        supportedTags.insert(QLatin1String("dd"));
        supportedTags.insert(QLatin1String("dfn"));
        supportedTags.insert(QLatin1String("em"));
        supportedTags.insert(QLatin1String("font"));
        supportedTags.insert(QLatin1String("h1"));
        supportedTags.insert(QLatin1String("h2"));
        supportedTags.insert(QLatin1String("h3"));
        supportedTags.insert(QLatin1String("h4"));
        supportedTags.insert(QLatin1String("h5"));
        supportedTags.insert(QLatin1String("h6"));
        supportedTags.insert(QLatin1String("head"));
        supportedTags.insert(QLatin1String("html"));
        supportedTags.insert(QLatin1String("meta"));
        supportedTags.insert(QLatin1String("nobr"));
        supportedTags.insert(QLatin1String("p"));
        supportedTags.insert(QLatin1String("pre"));
        supportedTags.insert(QLatin1String("qt"));
        supportedTags.insert(QLatin1String("s"));
        supportedTags.insert(QLatin1String("samp"));
        supportedTags.insert(QLatin1String("small"));
        supportedTags.insert(QLatin1String("span"));
        supportedTags.insert(QLatin1String("strong"));
        supportedTags.insert(QLatin1String("sub"));
        supportedTags.insert(QLatin1String("sup"));
        supportedTags.insert(QLatin1String("title"));
        supportedTags.insert(QLatin1String("var"));
        supportedTags.insert(QLatin1String("style"));
    }

    static QSet<QCss::Property> supportedCssProperties;
    if (supportedCssProperties.isEmpty()) {
        supportedCssProperties.insert(QCss::Color);
        supportedCssProperties.insert(QCss::Float);
        supportedCssProperties.insert(QCss::Font);
        supportedCssProperties.insert(QCss::FontFamily);
        supportedCssProperties.insert(QCss::FontSize);
        supportedCssProperties.insert(QCss::FontStyle);
        supportedCssProperties.insert(QCss::FontWeight);
        supportedCssProperties.insert(QCss::Margin);
        supportedCssProperties.insert(QCss::MarginBottom);
        supportedCssProperties.insert(QCss::MarginLeft);
        supportedCssProperties.insert(QCss::MarginRight);
        supportedCssProperties.insert(QCss::MarginTop);
        supportedCssProperties.insert(QCss::TextDecoration);
        supportedCssProperties.insert(QCss::TextIndent);
        supportedCssProperties.insert(QCss::TextUnderlineStyle);
        supportedCssProperties.insert(QCss::VerticalAlignment);
        supportedCssProperties.insert(QCss::Whitespace);
        supportedCssProperties.insert(QCss::Padding);
        supportedCssProperties.insert(QCss::PaddingLeft);
        supportedCssProperties.insert(QCss::PaddingRight);
        supportedCssProperties.insert(QCss::PaddingTop);
        supportedCssProperties.insert(QCss::PaddingBottom);
        supportedCssProperties.insert(QCss::PageBreakBefore);
        supportedCssProperties.insert(QCss::PageBreakAfter);
        supportedCssProperties.insert(QCss::Width);
        supportedCssProperties.insert(QCss::Height);
        supportedCssProperties.insert(QCss::MinimumWidth);
        supportedCssProperties.insert(QCss::MinimumHeight);
        supportedCssProperties.insert(QCss::MaximumWidth);
        supportedCssProperties.insert(QCss::MaximumHeight);
        supportedCssProperties.insert(QCss::Left);
        supportedCssProperties.insert(QCss::Right);
        supportedCssProperties.insert(QCss::Top);
        supportedCssProperties.insert(QCss::Bottom);
        supportedCssProperties.insert(QCss::Position);
        supportedCssProperties.insert(QCss::TextAlignment);
        supportedCssProperties.insert(QCss::FontVariant);
    }

    QXmlStreamReader reader(doc->toHtml("utf-8"));
    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isStartElement()) {
            if (!supportedTags.contains(reader.name().toString().toLower()))
                return true;

            QXmlStreamAttributes attributes = reader.attributes();
            if (attributes.hasAttribute(QLatin1String("bgcolor")))
                return true;
            if (attributes.hasAttribute(QLatin1String("style"))) {
                QCss::StyleSheet styleSheet;
                QCss::Parser(attributes.value(QLatin1String("style")).toString()).parse(&styleSheet);

                QVector<QCss::Declaration> decls;
                for (int i=0; i<styleSheet.pageRules.size(); ++i)
                    decls += styleSheet.pageRules.at(i).declarations;

                QVector<QCss::StyleRule> styleRules =
                        styleSheet.styleRules
                        + styleSheet.idIndex.values().toVector()
                        + styleSheet.nameIndex.values().toVector();
                for (int i=0; i<styleSheet.mediaRules.size(); ++i)
                    styleRules += styleSheet.mediaRules.at(i).styleRules;

                for (int i=0; i<styleRules.size(); ++i)
                    decls += styleRules.at(i).declarations;

                for (int i=0; i<decls.size(); ++i) {
                    if (!supportedCssProperties.contains(decls.at(i).d->propertyId))
                        return true;
                }

            }
        }
    }

    return reader.hasError();
}

void QSGTextNode::deleteContent()
{
    while (firstChild() != 0)
        delete firstChild();
    m_cursorNode = 0;
}

#if 0
void QSGTextNode::updateNodes()
{
    return;
    deleteContent();
    if (m_text.isEmpty())
        return;

    if (m_usePixmapCache) {
        // ### gunnar: port properly
//        QPixmap pixmap = generatedPixmap();
//        if (pixmap.isNull())
//            return;

//        QSGImageNode *pixmapNode = m_context->createImageNode();
//        pixmapNode->setRect(pixmap.rect());
//        pixmapNode->setSourceRect(pixmap.rect());
//        pixmapNode->setOpacity(m_opacity);
//        pixmapNode->setClampToEdge(true);
//        pixmapNode->setLinearFiltering(m_linearFiltering);

//        appendChildNode(pixmapNode);
    } else {
        if (m_text.isEmpty())
            return;

        // Implement styling by drawing text several times at slight shifts. shiftForStyle
        // contains the sequence of shifted positions at which to draw the text. All except
        // the last will be drawn with styleColor.
        QList<QPointF> shiftForStyle;
        switch (m_textStyle) {
        case OutlineTextStyle:
            // ### Should be made faster by implementing outline material
            shiftForStyle << QPointF(-1, 0);
            shiftForStyle << QPointF(0, -1);
            shiftForStyle << QPointF(1, 0);
            shiftForStyle << QPointF(0, 1);
            break;
        case SunkenTextStyle:
            shiftForStyle << QPointF(0, -1);
            break;
        case RaisedTextStyle:
            shiftForStyle << QPointF(0, 1);
            break;
        default:
            break;
        }

        shiftForStyle << QPointF(0, 0); // Regular position
        while (!shiftForStyle.isEmpty()) {
            QPointF shift = shiftForStyle.takeFirst();

            // Use styleColor for all but last shift
            if (m_richText) {
                QColor overrideColor = shiftForStyle.isEmpty() ? QColor() : m_styleColor;

                QTextFrame *textFrame = m_textDocument->rootFrame();
                QPointF p = m_textDocument->documentLayout()->frameBoundingRect(textFrame).topLeft();

                QTextFrame::iterator it = textFrame->begin();
                while (!it.atEnd()) {
                    addTextBlock(shift + p, it.currentBlock(), overrideColor);
                    ++it;
                }
            } else {
                addTextLayout(shift, m_textLayout, shiftForStyle.isEmpty()
                                                   ? m_color
                                                   : m_styleColor);
            }
        }
    }
}
#endif

QT_END_NAMESPACE
