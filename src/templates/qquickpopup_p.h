/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#ifndef QQUICKPOPUP_P_H
#define QQUICKPOPUP_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qmargins.h>
#include <QtGui/qevent.h>
#include <QtLabsTemplates/private/qtlabstemplatesglobal_p.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqmlparserstatus.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickPopupPrivate;
class QQuickTransition;
class QQuickTransform;

class Q_LABSTEMPLATES_EXPORT QQuickPopup : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged FINAL)
    Q_PROPERTY(qreal width READ width WRITE setWidth RESET resetWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(qreal height READ height WRITE setHeight RESET resetHeight NOTIFY heightChanged FINAL)
    Q_PROPERTY(qreal implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY implicitWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight WRITE setImplicitHeight NOTIFY implicitHeightChanged FINAL)
    Q_PROPERTY(qreal contentWidth READ contentWidth WRITE setContentWidth NOTIFY contentWidthChanged FINAL)
    Q_PROPERTY(qreal contentHeight READ contentHeight WRITE setContentHeight NOTIFY contentHeightChanged FINAL)
    Q_PROPERTY(qreal availableWidth READ availableWidth NOTIFY availableWidthChanged FINAL)
    Q_PROPERTY(qreal availableHeight READ availableHeight NOTIFY availableHeightChanged FINAL)
    Q_PROPERTY(qreal margins READ margins WRITE setMargins RESET resetMargins NOTIFY marginsChanged FINAL)
    Q_PROPERTY(qreal topMargin READ topMargin WRITE setTopMargin RESET resetTopMargin NOTIFY topMarginChanged FINAL)
    Q_PROPERTY(qreal leftMargin READ leftMargin WRITE setLeftMargin RESET resetLeftMargin NOTIFY leftMarginChanged FINAL)
    Q_PROPERTY(qreal rightMargin READ rightMargin WRITE setRightMargin RESET resetRightMargin NOTIFY rightMarginChanged FINAL)
    Q_PROPERTY(qreal bottomMargin READ bottomMargin WRITE setBottomMargin RESET resetBottomMargin NOTIFY bottomMarginChanged FINAL)
    Q_PROPERTY(qreal padding READ padding WRITE setPadding RESET resetPadding NOTIFY paddingChanged FINAL)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding RESET resetTopPadding NOTIFY topPaddingChanged FINAL)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding RESET resetLeftPadding NOTIFY leftPaddingChanged FINAL)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding RESET resetRightPadding NOTIFY rightPaddingChanged FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding RESET resetBottomPadding NOTIFY bottomPaddingChanged FINAL)
    Q_PROPERTY(QQuickItem *parent READ parentItem WRITE setParentItem NOTIFY parentChanged FINAL)
    Q_PROPERTY(QQuickItem *background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(QQuickItem *contentItem READ contentItem WRITE setContentItem NOTIFY contentItemChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QObject> contentData READ contentData FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickItem> contentChildren READ contentChildren NOTIFY contentChildrenChanged FINAL)
    Q_PROPERTY(bool focus READ hasFocus WRITE setFocus NOTIFY focusChanged)
    Q_PROPERTY(bool modal READ isModal WRITE setModal NOTIFY modalChanged)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(ClosePolicy closePolicy READ closePolicy WRITE setClosePolicy NOTIFY closePolicyChanged FINAL)
    Q_PROPERTY(TransformOrigin transformOrigin READ transformOrigin WRITE setTransformOrigin)
    Q_PROPERTY(QQuickTransition *enter READ enter WRITE setEnter NOTIFY enterChanged FINAL)
    Q_PROPERTY(QQuickTransition *exit READ exit WRITE setExit NOTIFY exitChanged FINAL)
    Q_CLASSINFO("DefaultProperty", "contentData")

public:
    explicit QQuickPopup(QObject *parent = Q_NULLPTR);
    ~QQuickPopup();

    qreal x() const;
    void setX(qreal x);

    qreal y() const;
    void setY(qreal y);

    qreal width() const;
    void setWidth(qreal width);
    void resetWidth();

    qreal height() const;
    void setHeight(qreal height);
    void resetHeight();

    qreal implicitWidth() const;
    void setImplicitWidth(qreal width);

    qreal implicitHeight() const;
    void setImplicitHeight(qreal height);

    qreal contentWidth() const;
    void setContentWidth(qreal width);

    qreal contentHeight() const;
    void setContentHeight(qreal height);

    qreal availableWidth() const;
    qreal availableHeight() const;

    qreal margins() const;
    void setMargins(qreal margins);
    void resetMargins();

    qreal topMargin() const;
    void setTopMargin(qreal margin);
    void resetTopMargin();

    qreal leftMargin() const;
    void setLeftMargin(qreal margin);
    void resetLeftMargin();

    qreal rightMargin() const;
    void setRightMargin(qreal margin);
    void resetRightMargin();

    qreal bottomMargin() const;
    void setBottomMargin(qreal margin);
    void resetBottomMargin();

    qreal padding() const;
    void setPadding(qreal padding);
    void resetPadding();

    qreal topPadding() const;
    void setTopPadding(qreal padding);
    void resetTopPadding();

    qreal leftPadding() const;
    void setLeftPadding(qreal padding);
    void resetLeftPadding();

    qreal rightPadding() const;
    void setRightPadding(qreal padding);
    void resetRightPadding();

    qreal bottomPadding() const;
    void setBottomPadding(qreal padding);
    void resetBottomPadding();

    QQuickItem *popupItem() const;

    QQuickItem *parentItem() const;
    void setParentItem(QQuickItem *parent);

    QQuickItem *background() const;
    void setBackground(QQuickItem *background);

    QQuickItem *contentItem() const;
    void setContentItem(QQuickItem *item);

    QQmlListProperty<QObject> contentData();
    QQmlListProperty<QQuickItem> contentChildren();

    bool hasFocus() const;
    void setFocus(bool focus);

    bool isModal() const;
    void setModal(bool modal);

    bool isVisible() const;
    void setVisible(bool visible);

    enum ClosePolicyFlag {
        NoAutoClose = 0x00,
        OnPressOutside = 0x01,
        OnPressOutsideParent = 0x02,
        OnReleaseOutside = 0x04,
        OnReleaseOutsideParent = 0x08,
        OnEscape = 0x10
    };
    Q_DECLARE_FLAGS(ClosePolicy, ClosePolicyFlag)
    Q_FLAG(ClosePolicy)

    ClosePolicy closePolicy() const;
    void setClosePolicy(ClosePolicy policy);

    // keep in sync with Item.TransformOrigin
    enum TransformOrigin {
        TopLeft, Top, TopRight,
        Left, Center, Right,
        BottomLeft, Bottom, BottomRight
    };
    Q_ENUM(TransformOrigin)

    TransformOrigin transformOrigin() const;
    void setTransformOrigin(TransformOrigin);

    QQuickTransition *enter() const;
    void setEnter(QQuickTransition *transition);

    QQuickTransition *exit() const;
    void setExit(QQuickTransition *transition);

    QQmlListProperty<QObject> data();

public Q_SLOTS:
    void open();
    void close();

Q_SIGNALS:
    void xChanged();
    void yChanged();
    void widthChanged();
    void heightChanged();
    void implicitWidthChanged();
    void implicitHeightChanged();
    void contentWidthChanged();
    void contentHeightChanged();
    void availableWidthChanged();
    void availableHeightChanged();
    void marginsChanged();
    void topMarginChanged();
    void leftMarginChanged();
    void rightMarginChanged();
    void bottomMarginChanged();
    void paddingChanged();
    void topPaddingChanged();
    void leftPaddingChanged();
    void rightPaddingChanged();
    void bottomPaddingChanged();
    void parentChanged();
    void backgroundChanged();
    void contentItemChanged();
    void contentChildrenChanged();
    void focusChanged();
    void modalChanged();
    void visibleChanged();
    void closePolicyChanged();
    void enterChanged();
    void exitChanged();

    void aboutToShow();
    void aboutToHide();

protected:
    QQuickPopup(QQuickPopupPrivate &dd, QObject *parent);

    void classBegin() Q_DECL_OVERRIDE;
    void componentComplete() Q_DECL_OVERRIDE;
    bool isComponentComplete() const;

    bool eventFilter(QObject *object, QEvent *event) Q_DECL_OVERRIDE;
    virtual void focusInEvent(QFocusEvent *event);
    virtual void focusOutEvent(QFocusEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void mouseUngrabEvent();
    virtual void wheelEvent(QWheelEvent *event);

    virtual void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem);
    virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    virtual void marginsChange(const QMarginsF &newMargins, const QMarginsF &oldMargins);
    virtual void paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding);

private:
    Q_DISABLE_COPY(QQuickPopup)
    Q_DECLARE_PRIVATE(QQuickPopup)
    friend class QQuickPopupItem;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickPopup::ClosePolicy)

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPopup)

#endif // QQUICKPOPUP_P_H
