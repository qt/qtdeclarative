/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktextnode_p.h"

#include <QtQuick/qsgsimplerectnode.h>
#include <private/qsgadaptationlayer_p.h>
#include <private/qsgdistancefieldglyphnode_p.h>
#include <private/qquickclipnode_p.h>
#include <QtQuick/private/qsgcontext_p.h>

#include <QtCore/qpoint.h>
#include <qmath.h>
#include <qtextdocument.h>
#include <qtextlayout.h>
#include <qabstracttextdocumentlayout.h>
#include <qxmlstream.h>
#include <qrawfont.h>
#include <qtexttable.h>
#include <qtextlist.h>
#include <private/qquickstyledtext_p.h>
#include <private/qquicktext_p_p.h>
#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#include <private/qrawfont_p.h>
#include <private/qtextimagehandler_p.h>
#include <private/qtextdocumentlayout_p.h>
#include <qhash.h>

QT_BEGIN_NAMESPACE

/*!
  Creates an empty QQuickTextNode
*/
QQuickTextNode::QQuickTextNode(QSGContext *context, QQuickItem *ownerElement)
    : m_context(context), m_cursorNode(0), m_ownerElement(ownerElement), m_useNativeRenderer(false)
{
#if defined(QML_RUNTIME_TESTING)
    description = QLatin1String("text");
#endif
}

QQuickTextNode::~QQuickTextNode()
{
    qDeleteAll(m_textures);
}

#if 0
void QQuickTextNode::setColor(const QColor &color)
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

void QQuickTextNode::setStyleColor(const QColor &styleColor)
{
    if (m_textStyle != QQuickTextNode::NormalTextStyle) {
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

QSGGlyphNode *QQuickTextNode::addGlyphs(const QPointF &position, const QGlyphRun &glyphs, const QColor &color,
                                     QQuickText::TextStyle style, const QColor &styleColor,
                                     QSGNode *parentNode)
{
    QSGGlyphNode *node = m_useNativeRenderer
            ? m_context->createNativeGlyphNode()
            : m_context->createGlyphNode();
    node->setOwnerElement(m_ownerElement);
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

void QQuickTextNode::setCursor(const QRectF &rect, const QColor &color)
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
            , decorations(QQuickTextNode::NoDecoration)
            , ascent(0.0)
            , leftChildIndex(-1)
            , rightChildIndex(-1)
        {

        }

        BinaryTreeNode(const QRectF &brect, const QImage &i, SelectionState selState, qreal a)
            : boundingRect(brect)
            , selectionState(selState)
            , clipNode(0)
            , decorations(QQuickTextNode::NoDecoration)
            , image(i)
            , ascent(a)
            , leftChildIndex(-1)
            , rightChildIndex(-1)
        {
        }

        BinaryTreeNode(const QGlyphRun &g, SelectionState selState, const QRectF &brect,
                       const QQuickTextNode::Decorations &decs, const QColor &c, const QColor &bc,
                       const QPointF &pos, qreal a)
            : glyphRun(g)
            , boundingRect(brect)
            , selectionState(selState)
            , clipNode(0)
            , decorations(decs)
            , color(c)
            , backgroundColor(bc)
            , position(pos)
            , ascent(a)
            , leftChildIndex(-1)
            , rightChildIndex(-1)
        {
        }

        QGlyphRun glyphRun;
        QRectF boundingRect;
        SelectionState selectionState;
        QQuickDefaultClipNode *clipNode;
        QQuickTextNode::Decorations decorations;
        QColor color;
        QColor backgroundColor;
        QPointF position;
        QImage image;
        qreal ascent;

        int leftChildIndex;
        int rightChildIndex;

        static void insert(QVarLengthArray<BinaryTreeNode> *binaryTree,
                           const QRectF &rect,
                           const QImage &image,
                           qreal ascent,
                           SelectionState selectionState)
        {
            insert(binaryTree, BinaryTreeNode(rect, image, selectionState, ascent));
        }

        static void insert(QVarLengthArray<BinaryTreeNode> *binaryTree,
                           const QGlyphRun &glyphRun,
                           SelectionState selectionState,
                           QQuickTextNode::Decorations decorations,
                           const QColor &textColor,
                           const QColor &backgroundColor,
                           const QPointF &position)
        {
            QRectF searchRect = glyphRun.boundingRect();
            searchRect.translate(position);

            if (qFuzzyIsNull(searchRect.width()) || qFuzzyIsNull(searchRect.height()))
                return;

            decorations |= (glyphRun.underline() ? QQuickTextNode::Underline : QQuickTextNode::NoDecoration);
            decorations |= (glyphRun.overline()  ? QQuickTextNode::Overline  : QQuickTextNode::NoDecoration);
            decorations |= (glyphRun.strikeOut() ? QQuickTextNode::StrikeOut : QQuickTextNode::NoDecoration);
            decorations |= (backgroundColor.isValid() ? QQuickTextNode::Background : QQuickTextNode::NoDecoration);

            qreal ascent = glyphRun.rawFont().ascent();
            insert(binaryTree, BinaryTreeNode(glyphRun, selectionState, searchRect, decorations,
                                              textColor, backgroundColor, position, ascent));
        }

        static void insert(QVarLengthArray<BinaryTreeNode> *binaryTree,
                           const BinaryTreeNode &binaryTreeNode)
        {
            int newIndex = binaryTree->size();
            binaryTree->append(binaryTreeNode);
            if (newIndex == 0)
                return;

            int searchIndex = 0;
            forever {
                BinaryTreeNode *node = binaryTree->data() + searchIndex;
                if (binaryTreeNode.boundingRect.left() < node->boundingRect.left()) {
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

        void addBorder(const QRectF &rect, qreal border, QTextFrameFormat::BorderStyle borderStyle,
                       const QBrush &borderBrush);
        void addFrameDecorations(QTextDocument *document, QTextFrame *frame);
        void addImage(const QRectF &rect, const QImage &image, qreal ascent,
                      BinaryTreeNode::SelectionState selectionState,
                      QTextFrameFormat::Position layoutPosition);
        int addText(const QTextBlock &block,
                    const QTextCharFormat &charFormat,
                    const QColor &textColor,
                    const QVarLengthArray<QTextLayout::FormatRange> &colorChanges,
                    int textPos, int fragmentEnd,
                    int selectionStart, int selectionEnd);
        void addTextObject(const QPointF &position, const QTextCharFormat &format,
                           BinaryTreeNode::SelectionState selectionState,
                           QTextDocument *textDocument, int pos,
                           QTextFrameFormat::Position layoutPosition = QTextFrameFormat::InFlow);
        void addSelectedGlyphs(const QGlyphRun &glyphRun);
        void addUnselectedGlyphs(const QGlyphRun &glyphRun);
        void addGlyphsInRange(int rangeStart, int rangeEnd,
                              const QColor &color, const QColor &backgroundColor,
                              int selectionStart, int selectionEnd);
        void addGlyphsForRanges(const QVarLengthArray<QTextLayout::FormatRange> &ranges,
                                int start, int end,
                                int selectionStart, int selectionEnd);

        void addToSceneGraph(QQuickTextNode *parent,
                             QQuickText::TextStyle style = QQuickText::Normal,
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

        void setAnchorColor(const QColor &anchorColor)
        {
            m_anchorColor = anchorColor;
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
        QColor m_anchorColor;
        QPointF m_position;

        QTextLine m_currentLine;
        bool m_hasSelection;

        QList<QPair<QRectF, QColor> > m_backgrounds;
        QList<QRectF> m_selectionRects;
        QVarLengthArray<BinaryTreeNode> m_currentLineTree;

        QList<TextDecoration> m_lines;
        QVector<BinaryTreeNode> m_processedNodes;

        QList<QPair<QRectF, QImage> > m_images;
    };

    int SelectionEngine::addText(const QTextBlock &block,
                                 const QTextCharFormat &charFormat,
                                 const QColor &textColor,
                                 const QVarLengthArray<QTextLayout::FormatRange> &colorChanges,
                                 int textPos, int fragmentEnd,
                                 int selectionStart, int selectionEnd)
    {
        if (charFormat.foreground().style() != Qt::NoBrush)
            setTextColor(charFormat.foreground().color());
        else
            setTextColor(textColor);

        while (textPos < fragmentEnd) {
            int blockRelativePosition = textPos - block.position();
            QTextLine line = block.layout()->lineForTextPosition(blockRelativePosition);
            if (!currentLine().isValid()
                || line.lineNumber() != currentLine().lineNumber()) {
                setCurrentLine(line);
            }

            Q_ASSERT(line.textLength() > 0);
            int lineEnd = line.textStart() + block.position() + line.textLength();

            int len = qMin(lineEnd - textPos, fragmentEnd - textPos);
            Q_ASSERT(len > 0);

            int currentStepEnd = textPos + len;

            addGlyphsForRanges(colorChanges,
                               textPos - block.position(),
                               currentStepEnd - block.position(),
                               selectionStart - block.position(),
                               selectionEnd - block.position());

            textPos = currentStepEnd;
        }
        return textPos;
    }

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

        QQuickTextNode::Decorations currentDecorations = QQuickTextNode::NoDecoration;
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
        if (!sortedIndexes.isEmpty()) {
            QQuickDefaultClipNode *currentClipNode = m_hasSelection ? new QQuickDefaultClipNode(QRectF()) : 0;
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
                if (currentDecorations != QQuickTextNode::NoDecoration) {
                    decorationRect.setY(m_position.y() + m_currentLine.y());
                    decorationRect.setHeight(m_currentLine.height());

                    if (node != 0)
                        decorationRect.setRight(node->boundingRect.left());

                    TextDecoration textDecoration(currentSelectionState, decorationRect, lastColor);
                    if (currentDecorations & QQuickTextNode::Underline)
                        pendingUnderlines.append(textDecoration);

                    if (currentDecorations & QQuickTextNode::Overline)
                        pendingOverlines.append(textDecoration);

                    if (currentDecorations & QQuickTextNode::StrikeOut)
                        pendingStrikeOuts.append(textDecoration);

                    if (currentDecorations & QQuickTextNode::Background)
                        m_backgrounds.append(qMakePair(decorationRect, lastBackgroundColor));
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
                            currentClipNode->setRect(currentRect);
                            currentClipNode->update();
                        }
                    }

                    if (node != 0 && m_hasSelection)
                        currentClipNode = new QQuickDefaultClipNode(QRectF());
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

                    // If previous item(s) had underline and current does not, then we add the
                    // pending lines to the lists and likewise for overlines and strikeouts
                    if (!pendingUnderlines.isEmpty()
                      && !(node->decorations & QQuickTextNode::Underline)) {
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
                    if (node->decorations & QQuickTextNode::Underline) {
                        if (rawFont.lineThickness() > underlineThickness) {
                            underlineThickness = rawFont.lineThickness();
                            underlineOffset = rawFont.underlinePosition();
                        }
                    }

                    if (node->decorations & QQuickTextNode::Overline) {
                        overlineOffset = -rawFont.ascent();
                        overlineThickness = rawFont.lineThickness();
                    }

                    if (node->decorations & QQuickTextNode::StrikeOut) {
                        strikeOutThickness = rawFont.lineThickness();
                        strikeOutOffset = rawFont.ascent() / -3.0;
                    }

                    currentDecorations = node->decorations;
                    lastColor = node->color;
                    lastBackgroundColor = node->backgroundColor;
                    m_processedNodes.append(*node);
                }
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

    void SelectionEngine::addImage(const QRectF &rect, const QImage &image, qreal ascent,
                                   BinaryTreeNode::SelectionState selectionState,
                                   QTextFrameFormat::Position layoutPosition)
    {
        QRectF searchRect = rect;
        if (layoutPosition == QTextFrameFormat::InFlow) {
            if (m_currentLineTree.isEmpty()) {
                searchRect.moveTopLeft(m_position + m_currentLine.position());
            } else {
                const BinaryTreeNode *lastNode = m_currentLineTree.data() + m_currentLineTree.size() - 1;
                if (lastNode->glyphRun.isRightToLeft()) {
                    QPointF lastPos = lastNode->boundingRect.topLeft();
                    searchRect.moveTopRight(lastPos - QPointF(0, ascent - lastNode->ascent));
                } else {
                    QPointF lastPos = lastNode->boundingRect.topRight();
                    searchRect.moveTopLeft(lastPos - QPointF(0, ascent - lastNode->ascent));
                }
            }
        }

        BinaryTreeNode::insert(&m_currentLineTree, searchRect, image, ascent, selectionState);
    }

    void SelectionEngine::addTextObject(const QPointF &position, const QTextCharFormat &format,
                                        BinaryTreeNode::SelectionState selectionState,
                                        QTextDocument *textDocument, int pos,
                                        QTextFrameFormat::Position layoutPosition)
    {
        QTextObjectInterface *handler = textDocument->documentLayout()->handlerForObject(format.objectType());
        if (handler != 0) {
            QImage image;
            QSizeF size = handler->intrinsicSize(textDocument, pos, format);

            if (format.objectType() == QTextFormat::ImageObject) {
                QTextImageFormat imageFormat = format.toImageFormat();
                if (QQuickTextDocumentWithImageResources *imageDoc = qobject_cast<QQuickTextDocumentWithImageResources *>(textDocument)) {
                    image = imageDoc->image(imageFormat);

                    if (image.isNull())
                        return;
                } else {
                    QTextImageHandler *imageHandler = static_cast<QTextImageHandler *>(handler);
                    image = imageHandler->image(textDocument, imageFormat);
                }
            }

            if (image.isNull()) {
                image = QImage(size.toSize(), QImage::Format_ARGB32_Premultiplied);
                image.fill(Qt::transparent);
                {
                    QPainter painter(&image);
                    handler->drawObject(&painter, image.rect(), textDocument, pos, format);
                }
            }

            qreal ascent;
            QFontMetrics m(format.font());
            switch (format.verticalAlignment())
            {
            case QTextCharFormat::AlignMiddle:
                ascent = size.height() / 2 - 1;
                break;
            case QTextCharFormat::AlignBaseline:
                ascent = size.height() - m.descent() - 1;
                break;
            default:
                ascent = size.height() - 1;
            }

            addImage(QRectF(position, size), image, ascent, selectionState, layoutPosition);
        }
    }

    void SelectionEngine::addUnselectedGlyphs(const QGlyphRun &glyphRun)
    {
        BinaryTreeNode::insert(&m_currentLineTree, glyphRun, BinaryTreeNode::Unselected,
                               QQuickTextNode::NoDecoration, m_textColor, m_backgroundColor, m_position);
    }

    void SelectionEngine::addSelectedGlyphs(const QGlyphRun &glyphRun)
    {
        int currentSize = m_currentLineTree.size();
        BinaryTreeNode::insert(&m_currentLineTree, glyphRun, BinaryTreeNode::Selected,
                               QQuickTextNode::NoDecoration, m_textColor, m_backgroundColor, m_position);
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
                QColor rangeColor;
                if (range.format.hasProperty(QTextFormat::ForegroundBrush))
                    rangeColor = range.format.foreground().color();
                else if (range.format.isAnchor())
                    rangeColor = m_anchorColor;
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

        bool hasSelection = selectionEnd >= 0
                         && selectionStart <= selectionEnd;

        QTextLine &line = m_currentLine;
        int rangeEnd = rangeStart + rangeLength;
        if (!hasSelection || (selectionStart > rangeEnd || selectionEnd < rangeStart)) {
            QList<QGlyphRun> glyphRuns = line.glyphRuns(rangeStart, rangeLength);
            for (int j=0; j<glyphRuns.size(); ++j) {
                const QGlyphRun &glyphRun = glyphRuns.at(j);
                addUnselectedGlyphs(glyphRun);
            }
        } else {
            if (rangeStart < selectionStart) {
                QList<QGlyphRun> glyphRuns = line.glyphRuns(rangeStart,
                                                            qMin(selectionStart - rangeStart,
                                                                 rangeLength));

                for (int j=0; j<glyphRuns.size(); ++j) {
                    const QGlyphRun &glyphRun = glyphRuns.at(j);
                    addUnselectedGlyphs(glyphRun);
                }
            }

            if (rangeEnd > selectionStart) {
                int start = qMax(selectionStart, rangeStart);
                int length = qMin(selectionEnd - start + 1, rangeEnd - start);
                QList<QGlyphRun> glyphRuns = line.glyphRuns(start, length);

                for (int j=0; j<glyphRuns.size(); ++j) {
                    const QGlyphRun &glyphRun = glyphRuns.at(j);
                    addSelectedGlyphs(glyphRun);
                }
            }

            if (selectionEnd >= rangeStart && selectionEnd < rangeEnd) {
                QList<QGlyphRun> glyphRuns = line.glyphRuns(selectionEnd + 1, rangeEnd - selectionEnd - 1);
                for (int j=0; j<glyphRuns.size(); ++j) {
                    const QGlyphRun &glyphRun = glyphRuns.at(j);
                    addUnselectedGlyphs(glyphRun);
                }
            }
        }

        if (backgroundColor.isValid())
            m_backgroundColor = oldBackgroundColor;

        if (oldColor.isValid())
            m_textColor = oldColor;
    }

    void SelectionEngine::addBorder(const QRectF &rect, qreal border,
                                    QTextFrameFormat::BorderStyle borderStyle,
                                    const QBrush &borderBrush)
    {
        QColor color = borderBrush.color();

        // Currently we don't support other styles than solid
        Q_UNUSED(borderStyle);

        m_backgrounds.append(qMakePair(QRectF(rect.left(), rect.top(), border, rect.height() + border), color));
        m_backgrounds.append(qMakePair(QRectF(rect.left() + border, rect.top(), rect.width(), border), color));
        m_backgrounds.append(qMakePair(QRectF(rect.right(), rect.top() + border, border, rect.height() - border), color));
        m_backgrounds.append(qMakePair(QRectF(rect.left() + border, rect.bottom(), rect.width(), border), color));
    }

    void SelectionEngine::addFrameDecorations(QTextDocument *document, QTextFrame *frame)
    {
        QTextDocumentLayout *documentLayout = qobject_cast<QTextDocumentLayout *>(document->documentLayout());
        QTextFrameFormat frameFormat = frame->format().toFrameFormat();

        QTextTable *table = qobject_cast<QTextTable *>(frame);
        QRectF boundingRect = table == 0
                ? documentLayout->frameBoundingRect(frame)
                : documentLayout->tableBoundingRect(table);

        QBrush bg = frame->frameFormat().background();
        if (bg.style() != Qt::NoBrush)
            m_backgrounds.append(qMakePair(boundingRect, bg.color()));

        if (!frameFormat.hasProperty(QTextFormat::FrameBorder))
            return;

        qreal borderWidth = frameFormat.border();
        if (qFuzzyIsNull(borderWidth))
            return;

        QBrush borderBrush = frameFormat.borderBrush();
        QTextFrameFormat::BorderStyle borderStyle = frameFormat.borderStyle();
        if (borderStyle == QTextFrameFormat::BorderStyle_None)
            return;

        addBorder(boundingRect.adjusted(frameFormat.leftMargin(), frameFormat.topMargin(),
                                        -frameFormat.rightMargin(), -frameFormat.bottomMargin()),
                  borderWidth, borderStyle, borderBrush);
        if (table != 0) {
            int rows = table->rows();
            int columns = table->columns();

            for (int row=0; row<rows; ++row) {
                for (int column=0; column<columns; ++column) {
                    QTextTableCell cell = table->cellAt(row, column);

                    QRectF cellRect = documentLayout->tableCellBoundingRect(table, cell);
                    addBorder(cellRect.adjusted(-borderWidth, -borderWidth, 0, 0), borderWidth,
                              borderStyle, borderBrush);
                }
            }
        }
    }

    void SelectionEngine::addToSceneGraph(QQuickTextNode *parentNode,
                                          QQuickText::TextStyle style,
                                          const QColor &styleColor)
    {
        if (m_currentLine.isValid())
            processCurrentLine();


        for (int i=0; i<m_backgrounds.size(); ++i) {
            const QRectF &rect = m_backgrounds.at(i).first;
            const QColor &color = m_backgrounds.at(i).second;

            parentNode->appendChildNode(new QSGSimpleRectNode(rect, color));
        }

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
        typedef QPair<QFontEngine *, QPair<QQuickDefaultClipNode *, QPair<QRgb, int> > > KeyType;
        QHash<KeyType, BinaryTreeNode *> map;
        QList<BinaryTreeNode *> nodes;
        for (int i=0; i<m_processedNodes.size(); ++i) {
            BinaryTreeNode *node = m_processedNodes.data() + i;

            if (node->image.isNull()) {
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
                    nodes.append(node);
                }
            } else {
                parentNode->addImage(node->boundingRect, node->image);
                if (node->selectionState == BinaryTreeNode::Selected) {
                    QColor color = m_selectionColor;
                    color.setAlpha(128);
                    parentNode->appendChildNode(new QSGSimpleRectNode(node->boundingRect, color));
                }
            }
        }

        // ...and add clip nodes and glyphs to tree.
        foreach (const BinaryTreeNode *node, nodes) {

            QQuickDefaultClipNode *clipNode = node->clipNode;
            if (clipNode != 0 && clipNode->parent() == 0 )
                parentNode->appendChildNode(clipNode);

            QColor color = node->selectionState == BinaryTreeNode::Selected
                    ? m_selectedTextColor
                    : node->color;

            parentNode->addGlyphs(node->position, node->glyphRun, color, style, styleColor, clipNode);
        }
    }
}

void QQuickTextNode::mergeFormats(QTextLayout *textLayout,
                               QVarLengthArray<QTextLayout::FormatRange> *mergedFormats)
{
    Q_ASSERT(mergedFormats != 0);
    if (textLayout == 0)
        return;

    QList<QTextLayout::FormatRange> additionalFormats = textLayout->additionalFormats();
    for (int i=0; i<additionalFormats.size(); ++i) {
        QTextLayout::FormatRange additionalFormat = additionalFormats.at(i);
        if (additionalFormat.format.hasProperty(QTextFormat::ForegroundBrush)
         || additionalFormat.format.hasProperty(QTextFormat::BackgroundBrush)
         || additionalFormat.format.isAnchor()) {
            // Merge overlapping formats
            if (!mergedFormats->isEmpty()) {
                QTextLayout::FormatRange *lastFormat = mergedFormats->data() + mergedFormats->size() - 1;

                if (additionalFormat.start < lastFormat->start + lastFormat->length) {
                    QTextLayout::FormatRange *mergedRange = 0;

                    int length = additionalFormat.length;
                    if (additionalFormat.start > lastFormat->start) {
                        lastFormat->length = additionalFormat.start - lastFormat->start;
                        length -= lastFormat->length;

                        mergedFormats->append(QTextLayout::FormatRange());
                        mergedRange = mergedFormats->data() + mergedFormats->size() - 1;
                        lastFormat = mergedFormats->data() + mergedFormats->size() - 2;
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
                mergedFormats->append(additionalFormat);
        }
    }

}

namespace {

    class ProtectedLayoutAccessor: public QAbstractTextDocumentLayout
    {
    public:
        inline QTextCharFormat formatAccessor(int pos)
        {
            return format(pos);
        }
    };

}

void QQuickTextNode::addImage(const QRectF &rect, const QImage &image)
{
    QSGImageNode *node = m_context->createImageNode();
    QSGTexture *texture = m_context->createTexture(image);
    m_textures.append(texture);
    node->setTargetRect(rect);
    node->setInnerTargetRect(rect);
    node->setTexture(texture);
    appendChildNode(node);
    node->update();
}

void QQuickTextNode::addTextDocument(const QPointF &position, QTextDocument *textDocument,
                                  const QColor &textColor,
                                  QQuickText::TextStyle style, const QColor &styleColor,
                                  const QColor &anchorColor,
                                  const QColor &selectionColor, const QColor &selectedTextColor,
                                  int selectionStart, int selectionEnd)
{
    SelectionEngine engine;
    engine.setTextColor(textColor);
    engine.setSelectedTextColor(selectedTextColor);
    engine.setSelectionColor(selectionColor);
    engine.setAnchorColor(anchorColor);

    QList<QTextFrame *> frames;
    frames.append(textDocument->rootFrame());
    while (!frames.isEmpty()) {
        QTextFrame *textFrame = frames.takeFirst();
        frames.append(textFrame->childFrames());

        engine.addFrameDecorations(textDocument, textFrame);

        if (textFrame->firstPosition() > textFrame->lastPosition()
         && textFrame->frameFormat().position() != QTextFrameFormat::InFlow) {
            const int pos = textFrame->firstPosition() - 1;
            ProtectedLayoutAccessor *a = static_cast<ProtectedLayoutAccessor *>(textDocument->documentLayout());
            QTextCharFormat format = a->formatAccessor(pos);
            QRectF rect = a->frameBoundingRect(textFrame);

            QTextBlock block = textFrame->firstCursorPosition().block();
            engine.setCurrentLine(block.layout()->lineForTextPosition(pos - block.position()));
            engine.addTextObject(rect.topLeft(), format, BinaryTreeNode::Unselected, textDocument,
                                 pos, textFrame->frameFormat().position());
        } else {
            QTextFrame::iterator it = textFrame->begin();

            while (!it.atEnd()) {
                Q_ASSERT(!engine.currentLine().isValid());

                QTextBlock block = it.currentBlock();
#ifndef QT_NO_IM
                int preeditLength = block.isValid() ? block.layout()->preeditAreaText().length() : 0;
                int preeditPosition = block.isValid() ? block.layout()->preeditAreaPosition() : -1;
#endif

                QVarLengthArray<QTextLayout::FormatRange> colorChanges;
                mergeFormats(block.layout(), &colorChanges);

                QPointF blockPosition = textDocument->documentLayout()->blockBoundingRect(block).topLeft() + position;
                if (QTextList *textList = block.textList()) {
                    QPointF pos = blockPosition;
                    QTextLayout *layout = block.layout();
                    if (layout->lineCount() > 0) {
                        QTextLine firstLine = layout->lineAt(0);
                        Q_ASSERT(firstLine.isValid());

                        engine.setCurrentLine(firstLine);

                        QRectF textRect = firstLine.naturalTextRect();
                        pos += textRect.topLeft();
                        if (block.textDirection() == Qt::RightToLeft)
                            pos.rx() += textRect.width();

                        const QTextCharFormat charFormat = block.charFormat();
                        QFont font(charFormat.font());
                        QFontMetricsF fontMetrics(font);
                        QTextListFormat listFormat = textList->format();

                        QString listItemBullet;
                        switch (listFormat.style()) {
                        case QTextListFormat::ListCircle:
                            listItemBullet = QChar(0x25E6); // White bullet
                            break;
                        case QTextListFormat::ListSquare:
                            listItemBullet = QChar(0x25AA); // Black small square
                            break;
                        case QTextListFormat::ListDecimal:
                        case QTextListFormat::ListLowerAlpha:
                        case QTextListFormat::ListUpperAlpha:
                        case QTextListFormat::ListLowerRoman:
                        case QTextListFormat::ListUpperRoman:
                            listItemBullet = textList->itemText(block);
                            break;
                        default:
                            listItemBullet = QChar(0x2022); // Black bullet
                            break;
                        };

                        QSizeF size(fontMetrics.width(listItemBullet), fontMetrics.height());
                        qreal xoff = fontMetrics.width(QLatin1Char(' '));
                        if (block.textDirection() == Qt::LeftToRight)
                            xoff = -xoff - size.width();
                        engine.setPosition(pos + QPointF(xoff, 0));

                        QTextLayout layout;
                        layout.setFont(font);
                        layout.setText(listItemBullet); // Bullet
                        layout.beginLayout();
                        QTextLine line = layout.createLine();
                        line.setPosition(QPointF(0, 0));
                        layout.endLayout();

                        QList<QGlyphRun> glyphRuns = layout.glyphRuns();
                        for (int i=0; i<glyphRuns.size(); ++i)
                            engine.addUnselectedGlyphs(glyphRuns.at(i));
                    }
                }

                int textPos = block.position();
                QTextBlock::iterator blockIterator = block.begin();

                while (!blockIterator.atEnd()) {
                    QTextFragment fragment = blockIterator.fragment();
                    QString text = fragment.text();
                    if (text.isEmpty())
                        continue;

                    QTextCharFormat charFormat = fragment.charFormat();
                    engine.setPosition(blockPosition);
                    if (text.contains(QChar::ObjectReplacementCharacter)) {
                        QTextFrame *frame = qobject_cast<QTextFrame *>(textDocument->objectForFormat(charFormat));
                        if (frame && frame->frameFormat().position() == QTextFrameFormat::InFlow) {
                            int blockRelativePosition = textPos - block.position();
                            QTextLine line = block.layout()->lineForTextPosition(blockRelativePosition);
                            if (!engine.currentLine().isValid()
                                    || line.lineNumber() != engine.currentLine().lineNumber()) {
                                engine.setCurrentLine(line);
                            }

                            BinaryTreeNode::SelectionState selectionState =
                                    (selectionStart < textPos + text.length()
                                     && selectionEnd >= textPos)
                                    ? BinaryTreeNode::Selected
                                    : BinaryTreeNode::Unselected;

                            engine.addTextObject(QPointF(), charFormat, selectionState, textDocument, textPos);
                        }
                        textPos += text.length();
                    } else {
                        if (charFormat.foreground().style() != Qt::NoBrush)
                            engine.setTextColor(charFormat.foreground().color());
                        else if (charFormat.isAnchor())
                            engine.setTextColor(anchorColor);
                        else
                            engine.setTextColor(textColor);

                        int fragmentEnd = textPos + fragment.length();
#ifndef QT_NO_IM
                        if (preeditPosition >= 0
                         && preeditPosition >= textPos
                         && preeditPosition <= fragmentEnd) {
                            fragmentEnd += preeditLength;
                        }
#endif

                        textPos = engine.addText(block, charFormat, textColor, colorChanges, textPos, fragmentEnd,
                                       selectionStart, selectionEnd);
                    }

                    ++blockIterator;
                }

#ifndef QT_NO_IM
                if (preeditLength >= 0 && textPos <= block.position() + preeditPosition) {
                    engine.setPosition(blockPosition);
                    textPos = block.position() + preeditPosition;
                    QTextLine line = block.layout()->lineForTextPosition(preeditPosition);
                    if (!engine.currentLine().isValid()
                            || line.lineNumber() != engine.currentLine().lineNumber()) {
                        engine.setCurrentLine(line);
                    }
                    textPos = engine.addText(block, block.charFormat(), textColor, colorChanges,
                                             textPos, textPos + preeditLength,
                                             selectionStart, selectionEnd);
                }
#endif

                engine.setCurrentLine(QTextLine()); // Reset current line because the text layout changed
                ++it;
            }
        }
    }

    engine.addToSceneGraph(this, style, styleColor);
}

void QQuickTextNode::addTextLayout(const QPointF &position, QTextLayout *textLayout, const QColor &color,
                                QQuickText::TextStyle style, const QColor &styleColor,
                                const QColor &anchorColor,
                                const QColor &selectionColor, const QColor &selectedTextColor,
                                int selectionStart, int selectionEnd,
                                int lineStart, int lineCount)
{
    SelectionEngine engine;
    engine.setTextColor(color);
    engine.setSelectedTextColor(selectedTextColor);
    engine.setSelectionColor(selectionColor);
    engine.setAnchorColor(anchorColor);
    engine.setPosition(position);

#ifndef QT_NO_IM
    int preeditLength = textLayout->preeditAreaText().length();
    int preeditPosition = textLayout->preeditAreaPosition();
#endif

    QVarLengthArray<QTextLayout::FormatRange> colorChanges;
    mergeFormats(textLayout, &colorChanges);

    lineCount = lineCount >= 0
            ? qMin(lineStart + lineCount, textLayout->lineCount())
            : textLayout->lineCount();

    for (int i=lineStart; i<lineCount; ++i) {
        QTextLine line = textLayout->lineAt(i);

        int start = line.textStart();
        int length = line.textLength();
        int end = start + length;

#ifndef QT_NO_IM
        if (preeditPosition >= 0
         && preeditPosition >= start
         && preeditPosition < end) {
            end += preeditLength;
        }
#endif

        engine.setCurrentLine(line);
        engine.addGlyphsForRanges(colorChanges, start, end, selectionStart, selectionEnd);
    }

    engine.addToSceneGraph(this, style, styleColor);
}

void QQuickTextNode::deleteContent()
{
    while (firstChild() != 0)
        delete firstChild();
    m_cursorNode = 0;
}

#if 0
void QQuickTextNode::updateNodes()
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
