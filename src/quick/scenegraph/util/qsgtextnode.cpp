// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgtextnode.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSGTextNode

    \brief The QSGTextNode class is a class for drawing text layouts and text documents in
    the Qt Quick scene graph.
    \inmodule QtQuick
    \since 6.7

    QSGTextNode can be useful for creating custom Qt Quick items that require text. It is used
    in Qt Quick by the Text, TextEdit and TextInput elements.

    You can create QSGTextNode objects using QQuickWindow::createTextNode(). The addTextLayout()
    and addTextDocument() functions provide ways to add text to the QSGTextNode. The text must
    already be laid out.

    \note Properties must be set before \l addTextLayout() or \l addTextDocument() are called in
    order to have an effect.

    \note The destruction of QSGTextNode has to be managed with care. In particular, since it
    references graphics resources, it must be deleted when the Qt Quick scene graph is invalidated.
    If the node is part of the graph and has the \c OwnedByParent flag set (which is the default),
    this will happen automatically. However, if the \c OwnedByParent flag is cleared and the node is
    disposed of manually, care must be taken to do this when the scene graph is invalidated. This
    can be done by connecting to the \l{QQuickWindow::sceneGraphInvalidated()} signal, or by
    implementing a slot in the QQuickItem subclass which is named \c{invalidateSceneGraph()}.
    See also the documentation of QQuickItem for more details.
 */

/*!
      \enum QSGTextNode::TextStyle

      This enum type describes styles that can be applied to text rendering.

      \value Normal The text is drawn without any style applied.
      \value Outline The text is drawn with an outline.
      \value Raised The text is drawn raised.
      \value Sunken The text is drawn sunken.

      \sa setTextStyle(), setStyleColor()
*/

/*!
      \enum QSGTextNode::RenderType

      This enum type describes type of glyph node used for rendering the text.

      \value QtRendering Text is rendered using a scalable distance field for each glyph.
      \value NativeRendering Text is rendered using a platform-specific technique.
      \value CurveRendering Text is rendered using a curve rasterizer running directly on the
                            graphics hardware.

      Select \c NativeRendering if you prefer text to look native on the target platform and do
      not require advanced features such as transformation of the text. Using such features in
      combination with the NativeRendering render type will lend poor and sometimes pixelated
      results.

      Both \c Text.QtRendering and \c Text.CurveRendering are hardware-accelerated techniques.
      \c QtRendering is the faster of the two, but uses more memory and will exhibit rendering
      artifacts at large sizes. \c CurveRendering should be considered as an alternative in cases
      where \c QtRendering does not give good visual results or where reducing graphics memory
      consumption is a priority.

      \sa setRenderType(), setRenderTypeQuality()
*/

/*!
    \fn void QSGTextNode::setColor(QColor color)

    Sets the main color to use when rendering the text to \a color.

    The default is black: \c QColor(0, 0, 0).
*/

/*!
    \fn QColor QSGTextNode::color() const

    Returns the main color used when rendering the text.
*/

/*!
    \fn void QSGTextNode::setStyleColor(QColor styleColor)

    Sets the style color to use when rendering the text to \a styleColor.

    The default is black: \c QColor(0, 0, 0).

    \sa setTextStyle()
*/

/*!
    \fn QColor QSGTextNode::styleColor() const

    Returns the style color used when rendering the text.

    \sa textStyle()
*/

/*!
    \fn void QSGTextNode::setTextStyle(QSGTextNode::TextStyle textStyle)

    Sets the style of the rendered text to \a textStyle. The default is \c Normal.

    \sa setStyleColor()
*/

/*!
    \fn QSGTextNode::TextStyle QSGTextNode::textStyle()

    Returns the style of the rendered text.

    \sa styleColor()
*/

/*!
    \fn void QSGTextNode::setLinkColor(QColor linkColor)

    Sets the color of or hyperlinks to \a linkColor in the text.

    The default is blue: \c QColor(0, 0, 255).
*/

/*!
    \fn QColor QSGTextNode::linkColor() const

    Returns the color of hyperlinks in the text.
*/

/*!
    \fn void QSGTextNode::setSelectionColor(QColor color)

    Sets the color of the selection background to \a color when any part of the text is
    marked as selected.

    The default is dark blue: \c QColor(0, 0, 128).
*/

/*!
    \fn QColor QSGTextNode::selectionColor() const

    Returns the color of the selection background when any part of the text is marked as selected.
*/

/*!
    \fn QColor QSGTextNode::selectionTextColor() const

    Returns the color of the selection text when any part of the text is marked as selected.
*/

/*!
    \fn void QSGTextNode::setSelectionTextColor(QColor selectionTextColor)

    Sets the color of the selection text to \a selectionTextColor when any part of the text is
    marked as selected.

    The default is white: \c QColor(255, 255, 255).
*/


/*!
    \fn void QSGTextNode::setRenderType(RenderType renderType)

    Sets the type of glyph node in use to \a renderType.

    The default is \l QtRendering.
*/

/*!
    \fn QSGTextNode::RenderType QSGTextNode::renderType() const

    Returns the type of glyph node used for rendering the text.
*/

/*!
    \fn void QSGTextNode::setRenderTypeQuality(int renderTypeQuality)

    If the \l renderType() in use supports it, set the quality to use when rendering the text.
    When supported, this can be used to trade visual fidelity for execution speed or memory.

    When the \a renderTypeQuality is < 0, the default quality is used.

    The \a renderTypeQuality can be any integer, although limitations imposed by the underlying
    graphics hardware may be encountered if extreme values are set. The Qt Quick Text element
    operates with the following predefined values:

    \value DefaultRenderTypeQuality    -1 (default)
    \value LowRenderTypeQuality        26
    \value NormalRenderTypeQuality     52
    \value HighRenderTypeQuality       104
    \value VeryHighRenderTypeQuality   208

    This value is currently only respected by the QtRendering render type. Setting it changes the
    resolution of the distance fields used to represent the glyphs. Setting it above normal will
    cause memory consumption to increase, but reduces filtering artifacts on very large text.

    The default is -1.
*/

/*!
    \fn int QSGTextNode::renderTypeQuality() const

    Returns the render type quality of the node. See \l setRenderTypeQuality() for details.
*/

/*!
    \fn void QSGTextNode::setFiltering(QSGTexture::Filtering filtering)

    Sets the sampling mode used when scaling images that are part of the displayed text to
    \a filtering. For smoothly scaled images, use \l{QSGTexture::Linear} here.

    The default is \l{QSGTexture::Nearest}.

    \sa filtering()
*/

/*!
    \fn QSGTexture::Filtering QSGTextNode::filtering() const

    Returns the sampling mode used when scaling images that are part of the displayed text.

    \sa setFiltering()
*/

/*!
    \fn void QSGTextNode::setViewport(const QRectF &viewport)

    Sets the bounding rect of the viewport where the text is displayed to \a viewport. Providing
    this information makes it possible for the QSGTextNode to optimize which parts of the text
    layout or document are included in the scene graph.

    The default is a default-constructed QRectF. For this viewport, all contents will be included
    in the graph.
*/

/*!
    \fn QRectF QSGTextNode::viewport() const

    Returns the current viewport set for this QSGTextNode.
*/

/*!
    \fn QSGTextNode::addTextLayout(QPointF position, QTextLayout *layout, int selectionStart = -1, int selectionCount = -1, int lineStart = 0, int lineCount = -1)

    Adds the contents of \a layout to the text node at \a position. If \a selectionStart is >= 0,
    then this marks the first character in a selected area of \a selectionCount number of
    characters. The selection is represented as a background fill with the \l selectionColor() and
    the selected text is rendered in the \l selectionTextColor().

    For convenience, \a lineStart and \a lineCount can be used to select the range of \l QTextLine
    objects to include from the layout. This can be useful, for instance, when creating elided
    layouts. If \a lineCount is < 0, then the the node will include the lines from \a lineStart to
    the end of the layout.

    This function forwards its arguments to the virtual function doAddTextLayout().

    \sa clear()
*/

/*!
    \fn QSGTextNode::addTextDocument(QPointF position, QTextDocument *document, int selectionStart = -1, int selectionCount = -1)

    Adds the contents of \a document to the text node at \a position. If \a selectionStart is >= 0,
    then this marks the first character in a selected area of \a selectionCount number of
    characters. The selection is represented as a background fill with the \l selectionColor() and
    the selected text is rendered in the \l selectionTextColor().

    This function forwards its arguments to the virtual function doAddTextDocument().

    \sa clear()
*/

/*!
    \fn QSGTextNode::doAddTextLayout(QPointF position, QTextLayout *layout, int selectionStart, int selectionCount, int lineStart, int lineCount)

    Virtual function called by addTextLayout(), which converts the contents of \a layout to scene
    graph nodes and adds them to the current node at \a position.

    If \a selectionStart is >= 0, then this marks the first character in a selected area of
    \a selectionCount number of characters. The selection is represented as a background fill with
    the \l selectionColor() and the selected text is rendered in the \l selectionTextColor().

    For convenience, \a lineStart and \a lineCount can be used to select the range of \l QTextLine
    objects to include from the layout. This can be useful, for instance, when creating elided
    layouts. If \a lineCount is < 0, then the the node will include the lines from \a lineStart to
    the end of the layout.

    \sa clear(), addTextLayout()
*/

/*!
    \fn QSGTextNode::doAddTextDocument(QPointF position, QTextDocument *document, int selectionStart, int selectionCount)

    Virtual function called by addTextDocument(), which converts the contents of \a document to
    scene graph nodes and adds them to the current node at \a position.

    If \a selectionStart is >= 0, then this marks the first character in a selected area of
    \a selectionCount number of characters. The selection is represented as a background fill with
    the \l selectionColor() and the selected text is rendered in the \l selectionTextColor().

    \sa clear(), addTextDocument()
*/

/*!
    \fn QSGTextNode::clear()

    Clears the contents of the node, deleting nodes and other data that represents the layouts
    and documents that have been added to it.

    \sa addTextLayout(), addTextDocument()
*/

QSGTextNode::~QSGTextNode() = default;

QT_END_NAMESPACE
