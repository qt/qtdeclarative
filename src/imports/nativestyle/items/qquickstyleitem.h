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

#ifndef QQUICKSTYLEITEM_H
#define QQUICKSTYLEITEM_H

#include <QtCore/qdebug.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>

#include "qquicknativestyle.h"
#include "qquickstyle.h"
#include "qquickstyleoption.h"

// Work-around for now, to avoid creator getting confused
// about missing macros. Should eventually be defined
// in qt declarative somewhere I assume.
#ifndef QML_NAMED_ELEMENT
#define QML_NAMED_ELEMENT(NAME)
#define QML_UNCREATABLE(NAME)
#endif

#ifdef QT_DEBUG
#define qqc2Debug() if (m_debugFlags.testFlag(Debug)) qDebug() << __FUNCTION__ << ":"
#define qqc2Info() if (m_debugFlags.testFlag(Info)) qDebug() << __FUNCTION__ << ":"
#define qqc2InfoHeading(HEADING) if (m_debugFlags.testFlag(Info)) qDebug() << "--------" << HEADING << "--------"
#else
#define qqc2Debug() if (false) qDebug()
#define qqc2Info() if (false) qDebug()
#define qqc2InfoHeading(HEADING) if (false) qDebug()
#endif

QT_BEGIN_NAMESPACE

using namespace QQC2;

class QQuickStyleMargins
{
    Q_GADGET

    Q_PROPERTY(int left READ left())
    Q_PROPERTY(int top READ top())
    Q_PROPERTY(int right READ right())
    Q_PROPERTY(int bottom READ bottom())

    QML_NAMED_ELEMENT(stylemargins)
    QML_UNCREATABLE("")

public:
    QQuickStyleMargins() {}
    QQuickStyleMargins(const QQuickStyleMargins &other) : m_margins(other.m_margins) {}
    QQuickStyleMargins(const QMargins &margins) : m_margins(margins) {}
    QQuickStyleMargins(const QRect &outer, const QRect &inner)
    {
        const int left = inner.left() - outer.left();
        const int top = inner.top() - outer.top();
        const int right = outer.right() - inner.right();
        const int bottom = outer.bottom() - inner.bottom();
        m_margins = QMargins(left, top, right, bottom);
    }

    inline void operator=(const QQuickStyleMargins &other) { m_margins = other.m_margins; }
    inline bool operator==(const QQuickStyleMargins &other) const { return other.m_margins == m_margins; }
    inline bool operator!=(const QQuickStyleMargins &other) const { return other.m_margins != m_margins; }

    inline int left() const { return m_margins.left(); }
    inline int right() const { return m_margins.right(); }
    inline int top() const { return m_margins.top(); }
    inline int bottom() const { return m_margins.bottom(); }

    QMargins m_margins;
};

QDebug operator<<(QDebug debug, const QQuickStyleMargins &padding);

struct StyleItemGeometry
{
    /*
    A QQuickItem is responsible for drawing a control, or a part of it.

    'minimumSize' should be the minimum possible size that the item can
    have _without_ taking content size into consideration (and still render
    correctly). This will also be the size of the image that the item is drawn
    to, unless QQuickStyleItem::useNinePatchImage is set to false. In that
    case, the size of the image will be set to the size of the item instead
    (which is set from QML, and will typically be the same as the size of the control).
    The default way to calculate minimumSize is to call style()->sizeFromContents()
    with an empty content size. This is not always well supported by the legacy QStyle
    implementations, which means that you might e.g get an empty size in return).
    For those cases, the correct solution is to go into the specific platform style
    and change it so that it returns a valid size also for this special case.

    'implicitSize' should reflect the preferred size of the item, taking the
    given content size (as set from QML) into account. But not all controls
    have contents (slider), and for many controls, the content/label is instead
    placed outside the item/background image (radiobutton). In both cases, the
    size of the item will not include the content size, and implicitSize can
    usually be set equal to minimumSize instead.

    'contentRect' should be the free space where the contents can be placed. Note that
    this rect doesn't need to have the same size as the contentSize provided as input
    to the style item. Instead, QStyle can typically calculate a rect that is bigger, to
    e.g center the contents inside the control.

    'layoutRect' can be set to shift the position of the whole control so
    that aligns correctly with the other controls. This is important for
    controls that draws e.g shadows or focus rings. Such adornments should
    be painted, but not be included when aligning the controls.
    */

    QSize minimumSize;
    QSize implicitSize;
    QRect contentRect;
    QRect layoutRect;   //If invalid, there are no layout margins!
    QMargins ninePatchMargins;
    qreal focusFrameRadius;
};

QDebug operator<<(QDebug debug, const StyleItemGeometry &cg);

class QQuickStyleItem : public QQuickItem
{
    Q_OBJECT

    // Input
    Q_PROPERTY(QQuickItem *control MEMBER m_control NOTIFY controlChanged)
    Q_PROPERTY(qreal contentWidth READ contentWidth WRITE setContentWidth)
    Q_PROPERTY(qreal contentHeight READ contentHeight WRITE setContentHeight)
    Q_PROPERTY(bool useNinePatchImage MEMBER m_useNinePatchImage)
    Q_PROPERTY(OverrideState overrideState MEMBER m_overrideState)

    // Output
    Q_PROPERTY(QQuickStyleMargins contentPadding READ contentPadding() NOTIFY contentPaddingChanged)
    Q_PROPERTY(QQuickStyleMargins layoutMargins READ layoutMargins() NOTIFY layoutMarginsChanged)
    Q_PROPERTY(QSize minimumSize READ minimumSize() NOTIFY minimumSizeChanged)
    Q_PROPERTY(int transitionDuration MEMBER m_transitionDuration CONSTANT)

    QML_NAMED_ELEMENT(StyleItem)
    QML_UNCREATABLE("StyleItem is an abstract base class.")

public:
    enum DirtyFlag {
        Nothing = 0,
        Geometry,
        Image,
        Everything = 255
    };
    Q_DECLARE_FLAGS(DirtyFlags, DirtyFlag)

    enum OverrideState {
        None = 0,
        AlwaysHovered,
        NeverHovered,
    };
    Q_ENUM(OverrideState)


#ifdef QT_DEBUG
    enum DebugFlag {
        NoDebug = 0x000,
        Debug = 0x001,
        Info = 0x002,
        ImageRect = 0x003,
        ContentRect = 0x008,
        LayoutRect = 0x010,
        Unscaled = 0x020,
        InputContentSize = 0x040,
        DontUseNinePatchImage = 0x080,
        NinePatchMargins = 0x100,
    };
    Q_FLAG(DebugFlag)
    Q_DECLARE_FLAGS(DebugFlags, DebugFlag)
#endif

    explicit QQuickStyleItem(QQuickItem *parent = nullptr);
    ~QQuickStyleItem() override;

    qreal contentWidth();
    void setContentWidth(qreal contentWidth);
    qreal contentHeight();
    void setContentHeight(qreal contentHeight);

    QQuickStyleMargins contentPadding() const;
    QQuickStyleMargins layoutMargins() const;
    QSize minimumSize() const;
    QSize imageSize() const;
    qreal focusFrameRadius() const;

    Q_INVOKABLE virtual QFont styleFont(QQuickItem *control) const;

    void markGeometryDirty();
    void markImageDirty();

signals:
    void controlChanged();
    void contentPaddingChanged();
    void layoutMarginsChanged();
    void fontChanged();
    void minimumSizeChanged();

protected:
    void componentComplete() override;
    QSGNode *updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void updatePolish() override;

    virtual void connectToControl() const;
    virtual void paintEvent(QPainter *painter) const = 0;
    virtual StyleItemGeometry calculateGeometry() = 0;

    static QStyle::State controlSize(QQuickItem *item);
    void initStyleOptionBase(QStyleOption &styleOption) const;

    inline QSize contentSize() const { return QSize(qCeil(m_contentSize.width()), qCeil(m_contentSize.height())); }
    inline static QStyle *style() { return QQuickNativeStyle::style(); }

    template <class T> inline const T* control() const {
#ifdef QT_DEBUG
        if (!dynamic_cast<T *>(m_control.data())) {
            qmlWarning(this) << "control property is not of correct type";
            Q_UNREACHABLE();
        }
#endif
        return static_cast<T *>(m_control.data());
    }

#ifdef QT_DEBUG
    DebugFlags m_debugFlags = NoDebug;
#endif
    OverrideState m_overrideState = None;

private:
    inline void updateGeometry();
    inline void paintControlToImage();

    int dprAlignedSize(const int size) const;

#ifdef QT_DEBUG
    void addDebugInfo();
#endif

private:
    QPointer<QQuickItem> m_control;
    QImage m_paintedImage;
    StyleItemGeometry m_styleItemGeometry;
    QSizeF m_contentSize;

    DirtyFlags m_dirty = Everything;
    bool m_useNinePatchImage = true;
    bool m_polishing = false;

#ifdef Q_OS_MACOS
    int m_transitionDuration = 150;
#else
    int m_transitionDuration = 400;
#endif

private:
    friend class QtQuickControls2MacOSStylePlugin;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickStyleItem::DirtyFlags)

#ifdef QT_DEBUG
Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickStyleItem::DebugFlags)
#endif

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickStyleItem)

#endif // QQUICKSTYLEITEM_H
