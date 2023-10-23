// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKTEXTEDIT_P_P_H
#define QQUICKTEXTEDIT_P_P_H

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

#include "qquicktextedit_p.h"
#include "qquickimplicitsizeitem_p_p.h"
#include "qquicktextutil_p.h"

#include <QtQml/qqml.h>
#include <QtCore/qlist.h>
#include <private/qlazilyallocated_p.h>

#if QT_CONFIG(accessibility)
#include <QtGui/qaccessible.h>
#endif

#include <limits>

QT_BEGIN_NAMESPACE
class QTextLayout;
class QQuickTextDocumentWithImageResources;
class QQuickTextControl;
class QQuickTextNode;
class QQuickTextNodeEngine;

class Q_QUICK_PRIVATE_EXPORT QQuickTextEditPrivate : public QQuickImplicitSizeItemPrivate
#if QT_CONFIG(accessibility)
    , public QAccessible::ActivationObserver
#endif
{
public:
    Q_DECLARE_PUBLIC(QQuickTextEdit)

    typedef QQuickTextEdit Public;

    struct Node {
        explicit Node(int startPos = std::numeric_limits<int>::max(),
                      QQuickTextNode *node = nullptr)
            : m_startPos(startPos), m_node(node), m_dirty(false) { }
        QQuickTextNode* textNode() const { return m_node; }
        void moveStartPos(int delta) { Q_ASSERT(m_startPos + delta > 0); m_startPos += delta; }
        int startPos() const { return m_startPos; }
        void setDirty() { m_dirty = true; }
        bool dirty() const { return m_dirty; }

    private:
        int m_startPos;
        QQuickTextNode* m_node;
        bool m_dirty;

#ifndef QT_NO_DEBUG_STREAM
        friend QDebug Q_QUICK_PRIVATE_EXPORT operator<<(QDebug, const Node &);
#endif
    };
    typedef QList<Node>::iterator TextNodeIterator;

    struct ExtraData {
        ExtraData();

        qreal padding;
        qreal topPadding;
        qreal leftPadding;
        qreal rightPadding;
        qreal bottomPadding;
        bool explicitTopPadding : 1;
        bool explicitLeftPadding : 1;
        bool explicitRightPadding : 1;
        bool explicitBottomPadding : 1;
        bool implicitResize : 1;
    };
    QLazilyAllocated<ExtraData> extra;


    QQuickTextEditPrivate()
        : color(QRgb(0xFF000000)), selectionColor(QRgb(0xFF000080)), selectedTextColor(QRgb(0xFFFFFFFF))
        , textMargin(0.0), xoff(0), yoff(0)
        , font(sourceFont), cursorComponent(nullptr), cursorItem(nullptr), document(nullptr), control(nullptr)
        , quickDocument(nullptr), lastSelectionStart(0), lastSelectionEnd(0), lineCount(0)
        , hAlign(QQuickTextEdit::AlignLeft), vAlign(QQuickTextEdit::AlignTop)
        , format(QQuickTextEdit::PlainText), wrapMode(QQuickTextEdit::NoWrap)
        , renderType(QQuickTextUtil::textRenderType<QQuickTextEdit>())
        , contentDirection(Qt::LayoutDirectionAuto)
        , mouseSelectionMode(QQuickTextEdit::SelectCharacters)
#if QT_CONFIG(im)
        , inputMethodHints(Qt::ImhNone)
#endif
        , updateType(UpdatePaintNode)
        , dirty(false), richText(false), cursorVisible(false), cursorPending(false)
        , focusOnPress(true), persistentSelection(false), requireImplicitWidth(false)
        , selectByMouse(true), canPaste(false), canPasteValid(false), hAlignImplicit(true)
        , textCached(true), inLayout(false), selectByKeyboard(false), selectByKeyboardSet(false)
        , hadSelection(false), markdownText(false), inResize(false)
    {
#if QT_CONFIG(accessibility)
        QAccessible::installActivationObserver(this);
#endif
    }

    ~QQuickTextEditPrivate()
    {
#if QT_CONFIG(accessibility)
        QAccessible::removeActivationObserver(this);
#endif
    }

    static QQuickTextEditPrivate *get(QQuickTextEdit *item) {
        return static_cast<QQuickTextEditPrivate *>(QObjectPrivate::get(item)); }

    void init();

    void resetInputMethod();
    void updateDefaultTextOption();
    void relayoutDocument();
    bool determineHorizontalAlignment();
    bool setHAlign(QQuickTextEdit::HAlignment, bool forceAlign = false);
    void mirrorChange() override;
    bool transformChanged(QQuickItem *transformedItem) override;
    qreal getImplicitWidth() const override;
    Qt::LayoutDirection textDirection(const QString &text) const;
    bool isLinkHoveredConnected();

#if QT_CONFIG(cursor)
    void updateMouseCursorShape();
#endif

    void setNativeCursorEnabled(bool) {}
    void handleFocusEvent(QFocusEvent *event);
    void addCurrentTextNodeToRoot(QQuickTextNodeEngine *, QSGTransformNode *, QQuickTextNode*, TextNodeIterator&, int startPos);
    QQuickTextNode* createTextNode();

#if QT_CONFIG(im)
    Qt::InputMethodHints effectiveInputMethodHints() const;
#endif

#if QT_CONFIG(accessibility)
    void accessibilityActiveChanged(bool active) override;
    QAccessible::Role accessibleRole() const override;
#endif

    inline qreal padding() const { return extra.isAllocated() ? extra->padding : 0.0; }
    void setTopPadding(qreal value, bool reset = false);
    void setLeftPadding(qreal value, bool reset = false);
    void setRightPadding(qreal value, bool reset = false);
    void setBottomPadding(qreal value, bool reset = false);

    bool isImplicitResizeEnabled() const;
    void setImplicitResizeEnabled(bool enabled);

    QColor color;
    QColor selectionColor;
    QColor selectedTextColor;

    QSizeF contentSize;

    qreal textMargin;
    qreal xoff;
    qreal yoff;

    QString text;
    QUrl baseUrl;
    QFont sourceFont;
    QFont font;

    QQmlComponent* cursorComponent;
    QQuickItem* cursorItem;
    QQuickTextDocumentWithImageResources *document;
    QQuickTextControl *control;
    QQuickTextDocument *quickDocument;
    QList<Node> textNodeMap;

    int lastSelectionStart;
    int lastSelectionEnd;
    int lineCount;
    int firstBlockInViewport = -1;   // only for the autotest; can be wrong after scrolling sometimes
    int firstBlockPastViewport = -1; // only for the autotest
    QRectF renderedRegion;

    enum UpdateType {
        UpdateNone,
        UpdateOnlyPreprocess,
        UpdatePaintNode,
        UpdateAll
    };

    QQuickTextEdit::HAlignment hAlign;
    QQuickTextEdit::VAlignment vAlign;
    QQuickTextEdit::TextFormat format;
    QQuickTextEdit::WrapMode wrapMode;
    QQuickTextEdit::RenderType renderType;
    Qt::LayoutDirection contentDirection;
    QQuickTextEdit::SelectionMode mouseSelectionMode;
#if QT_CONFIG(im)
    Qt::InputMethodHints inputMethodHints;
#endif
    UpdateType updateType;

    bool dirty : 1;
    bool richText : 1;
    bool cursorVisible : 1;
    bool cursorPending : 1;
    bool focusOnPress : 1;
    bool persistentSelection : 1;
    bool requireImplicitWidth:1;
    bool selectByMouse:1;
    bool canPaste:1;
    bool canPasteValid:1;
    bool hAlignImplicit:1;
    bool textCached:1;
    bool inLayout:1;
    bool selectByKeyboard:1;
    bool selectByKeyboardSet:1;
    bool hadSelection : 1;
    bool markdownText : 1;
    bool inResize : 1;

    static const int largeTextSizeThreshold;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug Q_QUICK_PRIVATE_EXPORT operator<<(QDebug debug, const QQuickTextEditPrivate::Node &);
#endif

QT_END_NAMESPACE

#endif // QQUICKTEXTEDIT_P_P_H
