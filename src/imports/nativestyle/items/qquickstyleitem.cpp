/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickstyleitem.h"

#include <QtQml/qqmlinfo.h>
#include <QtQuick/qsgninepatchnode.h>
#include <QtQuick/private/qquickwindow_p.h>

#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>

#include <QtQml/qqml.h>

#include "qquickstyleitembutton.h"
#include "qquickstylehelper_p.h"

QT_BEGIN_NAMESPACE

QStyle *QQuickStyleItem::s_style = nullptr;

QDebug operator<<(QDebug debug, const QQuickStyleMargins &padding)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "StyleMargins(";
    debug << padding.left() << ", ";
    debug << padding.top() << ", ";
    debug << padding.right() << ", ";
    debug << padding.bottom();
    debug << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const StyleItemGeometry &cg)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "StyleItemGeometry(";
    debug << "implicitSize:" << cg.implicitSize << ", ";
    debug << "contentRect:" << cg.contentRect << ", ";
    debug << "layoutRect:" << cg.layoutRect << ", ";
    debug << "minimumSize:" << cg.minimumSize;
    debug << ')';
    return debug;
}

QQuickStyleItem::QQuickStyleItem()
{
    setFlag(QQuickItem::ItemHasContents);
}

QQuickStyleItem::~QQuickStyleItem()
{
}

void QQuickStyleItem::connectToControl()
{
    connect(this, &QQuickStyleItem::enabledChanged, this, &QQuickStyleItem::markImageDirty);
    connect(this, &QQuickStyleItem::activeFocusChanged, this, &QQuickStyleItem::markImageDirty);
    connect(this, &QQuickStyleItem::windowChanged, this, &QQuickStyleItem::markImageDirty);
    connect(window(), &QQuickWindow::activeChanged, this, &QQuickStyleItem::markImageDirty);
}

void QQuickStyleItem::markImageDirty()
{
    m_dirty.setFlag(DirtyFlag::Image);
    polish();
}

void QQuickStyleItem::markGeometryDirty()
{
    m_dirty.setFlag(DirtyFlag::Geometry);
    polish();
}

QSGNode *QQuickStyleItem::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    QSGNinePatchNode *node = static_cast<QSGNinePatchNode *>(oldNode);
    if (!node)
        node = window()->createNinePatchNode();

    auto texture = window()->createTextureFromImage(m_paintedImage, QQuickWindow::TextureCanUseAtlas);
    const QSize padding = m_useNinePatchImage ? m_styleItemGeometry.minimumSize / 2 : QSize(0, 0);

    node->setTexture(texture);
    node->setBounds(boundingRect());
    node->setDevicePixelRatio(window()->devicePixelRatio());
    node->setPadding(padding.width(), padding.height(), padding.width(), padding.height());
    node->update();

    return node;
}

QStyle::State QQuickStyleItem::controlSize(QQuickItem *item)
{
    // TODO: add proper API for small and mini
    if (item->metaObject()->indexOfProperty("qqc2_style_small") != -1)
        return QStyle::State_Small;
    if (item->metaObject()->indexOfProperty("qqc2_style_mini") != -1)
        return QStyle::State_Mini;
    return QStyle::State_None;
}

void QQuickStyleItem::initStyleOptionBase(QStyleOption &styleOption)
{
    Q_ASSERT(m_control);

    styleOption.control = const_cast<QQuickItem *>(control<QQuickItem>());
    styleOption.window = window();
    styleOption.palette = QQuickItemPrivate::get(m_control)->palette()->toQPalette();
    styleOption.rect = QRect(QPoint(0, 0), m_styleItemGeometry.minimumSize);

    styleOption.state = QStyle::State_None;
    styleOption.state |= controlSize(styleOption.control);
    if (styleOption.window->isActive())
        styleOption.state |= QStyle::State_Active;

    // Note: not all controls inherit from QQuickControl (e.g QQuickTextField)
    if (const auto quickControl = dynamic_cast<QQuickControl *>(m_control.data())) {
        styleOption.direction = quickControl->isMirrored() ? Qt::RightToLeft : Qt::LeftToRight;
        if (quickControl->isEnabled())
            styleOption.state |= QStyle::State_Enabled;
        if (quickControl->hasVisualFocus())
            styleOption.state |= QStyle::State_HasFocus;
        if (quickControl->isUnderMouse())
            styleOption.state |= QStyle::State_MouseOver;
    }

    qqc2Debug() << styleOption;
}

void QQuickStyleItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    // Note that a change in geometry should not lead us into
    // recalculating the styling based geometry, as that will
    // change our implicitSize and controlSize, which will
    // cause a binding loop.
    if (!m_useNinePatchImage) {
        // Since the image has the same size as
        // the control, we need to repaint it.
        m_dirty.setFlag(DirtyFlag::Image);
        polish();
    } else {
        update();
    }
}

void QQuickStyleItem::updateGeometry()
{
    qqc2DebugHeading("GEOMETRY");
    m_dirty.setFlag(DirtyFlag::Geometry, false);

    const QQuickStyleMargins oldContentPadding = contentPadding();
    const QQuickStyleMargins oldInsets = insets();

    m_styleItemGeometry = calculateGeometry();

#ifdef QT_DEBUG
    if (m_styleItemGeometry.implicitSize.isEmpty())
        qmlWarning(this) << "implicitSize is empty!";
    if (m_styleItemGeometry.minimumSize.isEmpty())
        qmlWarning(this) << "minimumSize is empty!";
#endif

    if (contentPadding() != oldContentPadding)
        emit contentPaddingChanged();
    if (insets() != oldInsets)
        emit insetsChanged();

    setImplicitSize(m_styleItemGeometry.implicitSize.width(), m_styleItemGeometry.implicitSize.height());
    // Clear the dirty flag after setting implicit size, since the following call
    // to geometryChanged() might set it again, which is unnecessary.
    m_dirty.setFlag(DirtyFlag::Geometry, false);

    if (!m_useNinePatchImage)
        m_styleItemGeometry.minimumSize = size().toSize();

    qqc2Debug() << m_styleItemGeometry
                << "bounding rect:" << boundingRect()
                << "content padding:" << contentPadding()
                << "insets:" << insets()
                << "input content size:" << m_contentSize;
}

void QQuickStyleItem::paintControlToImage()
{
    qqc2DebugHeading("PAINT");
    if (m_styleItemGeometry.minimumSize.isEmpty())
        return;

    m_dirty.setFlag(DirtyFlag::Image, false);
    const qreal scale = window()->devicePixelRatio();
    m_paintedImage = QImage(m_styleItemGeometry.minimumSize * scale, QImage::Format_ARGB32_Premultiplied);
    m_paintedImage.setDevicePixelRatio(scale);
    m_paintedImage.fill(Qt::transparent);

    QPainter painter(&m_paintedImage);
    paintEvent(&painter);

#ifdef QT_DEBUG
    if (!m_debug.isEmpty())
        painter.fillRect(m_paintedImage.rect(), QColor(rand() % 255, rand() % 255, rand() % 255, 50));
#endif

    update();
}

void QQuickStyleItem::updatePolish()
{
    if (m_dirty.testFlag(DirtyFlag::Geometry))
        updateGeometry();
    if (m_dirty.testFlag(DirtyFlag::Image))
        paintControlToImage();
}

void QQuickStyleItem::componentComplete()
{
    Q_ASSERT_X(m_control, Q_FUNC_INFO, "You need to assign a value to property 'control'");

#ifdef QT_DEBUG
    if (qEnvironmentVariable("QQC2_USE_NINEPATCH_IMAGE") == QStringLiteral("false"))
        m_useNinePatchImage = false;
    if (qEnvironmentVariable("QQC2_DEBUG") == QStringLiteral("true")) {
        // Set the object name of any QML item to "debug" to print out
        // extra information about that item. Optionally add some extra
        // text to prefix the output (e.g "debug myButton").
        const QString prefix(QLatin1String("debug"));
        const QString name = m_control->objectName();
        if (name.startsWith(prefix)) {
            m_debug = m_control->objectName().mid(prefix.length() + 1);
            if (m_debug.isEmpty())
                m_debug = QStringLiteral("-");
        }
    }
#endif

    QQuickItem::componentComplete();
    connectToControl();
    polish();
}

qreal QQuickStyleItem::contentWidth()
{
    return m_contentSize.width();
}

void QQuickStyleItem::setContentWidth(qreal contentWidth)
{
    if (qFuzzyCompare(m_contentSize.width(), contentWidth))
        return;

    m_contentSize.setWidth(contentWidth);
    markGeometryDirty();
}

qreal QQuickStyleItem::contentHeight()
{
    return m_contentSize.height();
}

void QQuickStyleItem::setContentHeight(qreal contentHeight)
{
    if (qFuzzyCompare(m_contentSize.height(), contentHeight))
        return;

    m_contentSize.setHeight(contentHeight);
    markGeometryDirty();
}

QQuickStyleMargins QQuickStyleItem::contentPadding() const
{
    const QRect outerRect(QPoint(0, 0), m_styleItemGeometry.implicitSize);
    return QQuickStyleMargins(outerRect, m_styleItemGeometry.contentRect);
}

QQuickStyleMargins QQuickStyleItem::insets() const
{
    if (m_styleItemGeometry.layoutRect.isEmpty())
        return QQuickStyleMargins();
    const QRect innerRect(QPoint(0, 0), m_styleItemGeometry.implicitSize);
    return QQuickStyleMargins(m_styleItemGeometry.layoutRect, innerRect);
}

QFont QQuickStyleItem::styleFont(QQuickItem *control)
{
    Q_ASSERT(control);
    // Note: This function should be treated as if it was static
    // (meaning, don't assume anything in this object to be initialized).
    // Resolving the font/font size should be done early on from QML, before we get
    // around to calculate geometry and paint. Otherwise we typically need to do it
    // all over again when/if the font changes. In practice this means that other
    // items in QML that uses a style font, and at the same time, affects our input
    // contentSize, cannot wait for this item to be fully constructed before it
    // gets the font. So we need to resolve it here and now, even if this
    // object might be in a half initialized state (hence also the control
    // argument, instead of relying on m_control to be set).
    return QGuiApplication::font();
}

QT_END_NAMESPACE
