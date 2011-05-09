/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
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
        for (int i=0; i<childCount(); ++i) {
            QSGNode *childNode = childAtIndex(i);
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
            for (int i=0; i<childCount(); ++i) {
                QSGNode *childNode = childAtIndex(i);
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

void QSGTextNode::addTextDecorations(const QPointF &position, const QRawFont &font, const QColor &color,
                                     qreal width, bool hasOverline, bool hasStrikeOut, bool hasUnderline)
{
    Q_ASSERT(font.isValid());
    QRawFontPrivate *dptrFont = QRawFontPrivate::get(font);
    QFontEngine *fontEngine = dptrFont->fontEngine;

    qreal lineThickness = fontEngine->lineThickness().toReal();

    QRectF line(position.x(), position.y() - lineThickness / 2.0, width, lineThickness);

    if (hasUnderline) {
        int underlinePosition = fontEngine->underlinePosition().ceil().toInt();
        QRectF underline(line);
        underline.translate(0.0, underlinePosition);
        appendChildNode(new QSGSimpleRectNode(underline, color));
    }

    qreal ascent = font.ascent();
    if (hasOverline) {
        QRectF overline(line);
        overline.translate(0.0, -ascent);
        appendChildNode(new QSGSimpleRectNode(overline, color));
    }

    if (hasStrikeOut) {
        QRectF strikeOut(line);
        strikeOut.translate(0.0, ascent / -3.0);
        appendChildNode(new QSGSimpleRectNode(strikeOut, color));
    }
}

QSGGlyphNode *QSGTextNode::addGlyphs(const QPointF &position, const QGlyphRun &glyphs, const QColor &color,
                                           QSGText::TextStyle style, const QColor &styleColor)
{
    QSGGlyphNode *node = m_context->createGlyphNode();
    if (QSGDistanceFieldGlyphCache::distanceFieldEnabled()) {
        QSGDistanceFieldGlyphNode *dfNode = static_cast<QSGDistanceFieldGlyphNode *>(node);
        dfNode->setStyle(style);
        dfNode->setStyleColor(styleColor);
    }
    node->setGlyphs(position, glyphs);
    node->setColor(color);

    appendChildNode(node);

    return node;
}

void QSGTextNode::addTextDocument(const QPointF &position, QTextDocument *textDocument, const QColor &color,
                                  QSGText::TextStyle style, const QColor &styleColor)
{
    Q_UNUSED(position)
    QTextFrame *textFrame = textDocument->rootFrame();
    QPointF p = textDocument->documentLayout()->frameBoundingRect(textFrame).topLeft();

    QTextFrame::iterator it = textFrame->begin();
    while (!it.atEnd()) {
        addTextBlock(p, textDocument, it.currentBlock(), color, style, styleColor);
        ++it;
    }
}

void QSGTextNode::setCursor(const QRectF &rect, const QColor &color)
{
    if (m_cursorNode != 0)
        delete m_cursorNode;

    m_cursorNode = new QSGSimpleRectNode(rect, color);
    appendChildNode(m_cursorNode);
}

QSGGlyphNode *QSGTextNode::addGlyphsAndDecoration(const QPointF &position, const QGlyphRun &glyphRun,
                                         const QColor &color, QSGText::TextStyle style,
                                         const QColor &styleColor,
                                         const QPointF &decorationPosition)
{
    QSGGlyphNode *node = addGlyphs(position, glyphRun, color, style, styleColor);

    if (glyphRun.strikeOut() || glyphRun.overline() || glyphRun.underline()) {
        QRectF rect = glyphRun.boundingRect();
        qreal width = rect.right() - decorationPosition.x();

        addTextDecorations(decorationPosition, glyphRun.rawFont(), color, width,
                           glyphRun.overline(), glyphRun.strikeOut(), glyphRun.underline());
    }

    return node;
}

namespace {

    struct BinaryTreeNode {
        enum SelectionState {
            Unselected,
            Selected
        };

        BinaryTreeNode()
            : selectionState(Unselected)
            , leftChildIndex(-1)
            , rightChildIndex(-1)
        {

        }

        BinaryTreeNode(const QGlyphRun &g, SelectionState selState, const QRectF &brect)
            : glyphRun(g)
            , boundingRect(brect)
            , selectionState(selState)
            , leftChildIndex(-1)
            , rightChildIndex(-1)
        {
        }

        QGlyphRun glyphRun;
        QRectF boundingRect;
        SelectionState selectionState;

        int leftChildIndex;
        int rightChildIndex;

        static void insert(QVarLengthArray<BinaryTreeNode> *binaryTree,
                           const QGlyphRun &glyphRun,
                           SelectionState selectionState)
        {
            int newIndex = binaryTree->size();
            QRectF searchRect = glyphRun.boundingRect();

            binaryTree->append(BinaryTreeNode(glyphRun, selectionState, searchRect));
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
}

void QSGTextNode::addTextLayout(const QPointF &position, QTextLayout *textLayout, const QColor &color,
                                QSGText::TextStyle style, const QColor &styleColor,
                                const QColor &selectionColor, const QColor &selectedTextColor,
                                int selectionStart, int selectionEnd)
{
    QFont font = textLayout->font();
    bool overline = font.overline();
    bool underline = font.underline();
    bool strikeOut = font.strikeOut();
    QRawFont decorationRawFont = QRawFont::fromFont(font);

    if (selectionStart < 0 || selectionEnd < 0) {
        for (int i=0; i<textLayout->lineCount(); ++i) {
            QTextLine line = textLayout->lineAt(i);

            QList<QGlyphRun> glyphRuns = line.glyphRuns();
            for (int j=0; j<glyphRuns.size(); ++j) {
                QGlyphRun glyphRun = glyphRuns.at(j);
                QPointF pos(position + QPointF(0, glyphRun.rawFont().ascent()));
                addGlyphs(pos, glyphRun, color, style, styleColor);
            }

            addTextDecorations(QPointF(line.x(), line.y() + decorationRawFont.ascent()),
                               decorationRawFont, color, line.naturalTextWidth(),
                               overline, strikeOut, underline);
        }

    } else {
        QVarLengthArray<BinaryTreeNode> binaryNodes;
        for (int i=0; i<textLayout->lineCount(); ++i) {
            QTextLine line = textLayout->lineAt(i);

            // Make a list of glyph runs sorted on left-most x coordinate to make it possible
            // to find the correct selection rects
            if (line.textStart() < selectionStart) {
                QList<QGlyphRun> glyphRuns = line.glyphRuns(line.textStart(),
                                                            qMin(selectionStart - line.textStart(),
                                                                 line.textLength()));
                for (int j=0; j<glyphRuns.size(); ++j) {
                    const QGlyphRun &glyphRun = glyphRuns.at(j);
                    BinaryTreeNode::insert(&binaryNodes, glyphRun, BinaryTreeNode::Unselected);
                }
            }

            int lineEnd = line.textStart() + line.textLength();

            // Add selected text
            if (lineEnd >= selectionStart && selectionStart >= line.textStart()) {
                QList<QGlyphRun> selectedGlyphRuns = line.glyphRuns(selectionStart,
                                                                    selectionEnd - selectionStart + 1);
                for (int j=0; j<selectedGlyphRuns.size(); ++j) {
                    const QGlyphRun &selectedGlyphRun = selectedGlyphRuns.at(j);
                    BinaryTreeNode::insert(&binaryNodes, selectedGlyphRun, BinaryTreeNode::Selected);
                }
            }

            // If there is selected text in this line, add regular text after selection
            if (selectionEnd >= line.textStart() && selectionEnd < lineEnd) {
                QList<QGlyphRun> glyphRuns = line.glyphRuns(selectionEnd + 1, lineEnd - selectionEnd);
                for (int j=0; j<glyphRuns.size(); ++j) {
                    const QGlyphRun &glyphRun = glyphRuns.at(j);
                    BinaryTreeNode::insert(&binaryNodes, glyphRun, BinaryTreeNode::Unselected);
                }
            }

            // Go through glyph runs sorted by x position and add glyph nodes/text decoration
            // and selection rects to the graph
            QVarLengthArray<int> sortedIndexes;
            BinaryTreeNode::inOrder(binaryNodes, &sortedIndexes);

            Q_ASSERT(sortedIndexes.size() == binaryNodes.size());

            BinaryTreeNode::SelectionState currentSelectionState = BinaryTreeNode::Unselected;
            QRectF currentRect = QRectF(line.x(), line.y(), 1, 1);
            for (int i=0; i<=sortedIndexes.size(); ++i) {
                BinaryTreeNode *node = 0;
                if (i < sortedIndexes.size()) {
                    int sortedIndex = sortedIndexes.at(i);
                    Q_ASSERT(sortedIndex < binaryNodes.size());

                    node = binaryNodes.data() + sortedIndex;
                    const QGlyphRun &glyphRun = node->glyphRun;

                    QColor currentColor = node->selectionState == BinaryTreeNode::Unselected
                            ? color
                            : selectedTextColor;

                    QPointF pos(position + QPointF(0, glyphRun.rawFont().ascent()));
                    addGlyphs(pos, glyphRun, currentColor, style, styleColor);

                    if (i == 0)
                        currentSelectionState = node->selectionState;
                }

                // If we've reached an unselected node from a selected node, we add the
                // selection rect to the graph, and we add decoration every time the
                // selection state changes, because that means the text color changes
                if (node == 0 || node->selectionState != currentSelectionState) {
                    if (node != 0)
                        currentRect.setRight(node->boundingRect.left());
                    else
                        currentRect.setWidth(line.naturalTextWidth() - (currentRect.x() - line.x()));
                    currentRect.setY(line.y());
                    currentRect.setHeight(line.height());

                    // Draw selection all the way up to the left edge of the unselected item
                    if (currentSelectionState == BinaryTreeNode::Selected)
                        prependChildNode(new QSGSimpleRectNode(currentRect, selectionColor));

                    if (overline || underline || strikeOut) {
                        QColor currentColor = currentSelectionState == BinaryTreeNode::Unselected
                                ? color
                                : selectedTextColor;
                        addTextDecorations(currentRect.topLeft() + QPointF(0, decorationRawFont.ascent()),
                                           decorationRawFont, currentColor, currentRect.width(),
                                           overline, strikeOut, underline);
                    }

                    if (node != 0) {
                        currentSelectionState = node->selectionState;
                        currentRect = node->boundingRect;
                    }
                } else {
                    if (currentRect.isEmpty())
                        currentRect = node->boundingRect;
                    else
                        currentRect = currentRect.united(node->boundingRect);
                }
            }
        }
    }
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

void QSGTextNode::addTextBlock(const QPointF &position, QTextDocument *textDocument, const QTextBlock &block,
                               const QColor &overrideColor, QSGText::TextStyle style, const QColor &styleColor)
{
    if (!block.isValid())
        return;

    QPointF blockPosition = textDocument->documentLayout()->blockBoundingRect(block).topLeft();

    QTextBlock::iterator it = block.begin();
    while (!it.atEnd()) {
        QTextFragment fragment = it.fragment();
        if (!fragment.text().isEmpty()) {
            QTextCharFormat charFormat = fragment.charFormat();
            QColor color = overrideColor.isValid()
                    ? overrideColor
                    : charFormat.foreground().color();

            QList<QGlyphRun> glyphsList = fragment.glyphRuns();
            for (int i=0; i<glyphsList.size(); ++i) {
                QGlyphRun glyphs = glyphsList.at(i);
                QRawFont font = glyphs.rawFont();
                QSGGlyphNode *glyphNode = addGlyphs(position + blockPosition + QPointF(0, font.ascent()),
                                                    glyphs, color, style, styleColor);

                QPointF baseLine = glyphNode->baseLine();
                qreal width = glyphNode->boundingRect().width();
                addTextDecorations(baseLine, font, color, width,
                                   glyphs.overline(), glyphs.strikeOut(), glyphs.underline());
            }
        }

        ++it;
    }
}

void QSGTextNode::deleteContent()
{
    while (childCount() > 0)
        delete childAtIndex(0);
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
