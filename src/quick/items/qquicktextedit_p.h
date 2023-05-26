// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKTEXTEDIT_P_H
#define QQUICKTEXTEDIT_P_H

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

#include "qquickimplicitsizeitem_p.h"
#include "qquicktextinterface_p.h"

#include <QtGui/qtextoption.h>

QT_BEGIN_NAMESPACE

class QQuickTextDocument;
class QQuickTextEditPrivate;
class QTextBlock;

class Q_QUICK_PRIVATE_EXPORT QQuickTextEdit : public QQuickImplicitSizeItem, public QQuickTextInterface
{
    Q_OBJECT
    Q_INTERFACES(QQuickTextInterface)

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged FINAL)
    Q_PROPERTY(QColor selectedTextColor READ selectedTextColor WRITE setSelectedTextColor NOTIFY selectedTextColorChanged FINAL)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(HAlignment horizontalAlignment READ hAlign WRITE setHAlign RESET resetHAlign NOTIFY horizontalAlignmentChanged FINAL)
    Q_PROPERTY(HAlignment effectiveHorizontalAlignment READ effectiveHAlign NOTIFY effectiveHorizontalAlignmentChanged FINAL)
    Q_PROPERTY(VAlignment verticalAlignment READ vAlign WRITE setVAlign NOTIFY verticalAlignmentChanged FINAL)
    Q_PROPERTY(WrapMode wrapMode READ wrapMode WRITE setWrapMode NOTIFY wrapModeChanged FINAL)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY lineCountChanged FINAL)
    Q_PROPERTY(int length READ length NOTIFY textChanged FINAL)
    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY contentSizeChanged FINAL)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY contentSizeChanged FINAL)
    Q_PROPERTY(qreal paintedWidth READ contentWidth NOTIFY contentSizeChanged FINAL)  // Compatibility
    Q_PROPERTY(qreal paintedHeight READ contentHeight NOTIFY contentSizeChanged FINAL)
    Q_PROPERTY(TextFormat textFormat READ textFormat WRITE setTextFormat NOTIFY textFormatChanged FINAL)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged FINAL)
    Q_PROPERTY(bool cursorVisible READ isCursorVisible WRITE setCursorVisible NOTIFY cursorVisibleChanged FINAL)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged FINAL)
    Q_PROPERTY(QRectF cursorRectangle READ cursorRectangle NOTIFY cursorRectangleChanged FINAL)
    Q_PROPERTY(QQmlComponent* cursorDelegate READ cursorDelegate WRITE setCursorDelegate NOTIFY cursorDelegateChanged FINAL)
    Q_PROPERTY(bool overwriteMode READ overwriteMode WRITE setOverwriteMode NOTIFY overwriteModeChanged FINAL)
    Q_PROPERTY(int selectionStart READ selectionStart NOTIFY selectionStartChanged FINAL)
    Q_PROPERTY(int selectionEnd READ selectionEnd NOTIFY selectionEndChanged FINAL)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectedTextChanged FINAL)
    Q_PROPERTY(bool activeFocusOnPress READ focusOnPress WRITE setFocusOnPress NOTIFY activeFocusOnPressChanged FINAL)
    Q_PROPERTY(bool persistentSelection READ persistentSelection WRITE setPersistentSelection NOTIFY persistentSelectionChanged FINAL)
    Q_PROPERTY(qreal textMargin READ textMargin WRITE setTextMargin NOTIFY textMarginChanged FINAL)
    Q_PROPERTY(Qt::InputMethodHints inputMethodHints READ inputMethodHints WRITE setInputMethodHints NOTIFY inputMethodHintsChanged FINAL)
    Q_PROPERTY(bool selectByKeyboard READ selectByKeyboard WRITE setSelectByKeyboard NOTIFY selectByKeyboardChanged REVISION(2, 1) FINAL)
    Q_PROPERTY(bool selectByMouse READ selectByMouse WRITE setSelectByMouse NOTIFY selectByMouseChanged FINAL)
    Q_PROPERTY(SelectionMode mouseSelectionMode READ mouseSelectionMode WRITE setMouseSelectionMode NOTIFY mouseSelectionModeChanged FINAL)
    Q_PROPERTY(bool canPaste READ canPaste NOTIFY canPasteChanged FINAL)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged FINAL)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged FINAL)
    Q_PROPERTY(bool inputMethodComposing READ isInputMethodComposing NOTIFY inputMethodComposingChanged FINAL)
    Q_PROPERTY(QUrl baseUrl READ baseUrl WRITE setBaseUrl RESET resetBaseUrl NOTIFY baseUrlChanged FINAL)
    Q_PROPERTY(RenderType renderType READ renderType WRITE setRenderType NOTIFY renderTypeChanged FINAL)
    Q_PROPERTY(QQuickTextDocument *textDocument READ textDocument CONSTANT FINAL REVISION(2, 1))
    Q_PROPERTY(QString hoveredLink READ hoveredLink NOTIFY linkHovered REVISION(2, 2) FINAL)
    Q_PROPERTY(qreal padding READ padding WRITE setPadding RESET resetPadding NOTIFY paddingChanged REVISION(2, 6) FINAL)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding RESET resetTopPadding NOTIFY topPaddingChanged REVISION(2, 6) FINAL)
    Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding RESET resetLeftPadding NOTIFY leftPaddingChanged REVISION(2, 6) FINAL)
    Q_PROPERTY(qreal rightPadding READ rightPadding WRITE setRightPadding RESET resetRightPadding NOTIFY rightPaddingChanged REVISION(2, 6) FINAL)
    Q_PROPERTY(qreal bottomPadding READ bottomPadding WRITE setBottomPadding RESET resetBottomPadding NOTIFY bottomPaddingChanged REVISION(2, 6) FINAL)
    Q_PROPERTY(QString preeditText READ preeditText NOTIFY preeditTextChanged REVISION(2, 7) FINAL)
    Q_PROPERTY(qreal tabStopDistance READ tabStopDistance WRITE setTabStopDistance NOTIFY tabStopDistanceChanged REVISION(2, 10) FINAL)
    QML_NAMED_ELEMENT(TextEdit)
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    QML_ADDED_IN_VERSION(6, 4)
#endif

public:
    QQuickTextEdit(QQuickItem *parent=nullptr);

    enum HAlignment {
        AlignLeft = Qt::AlignLeft,
        AlignRight = Qt::AlignRight,
        AlignHCenter = Qt::AlignHCenter,
        AlignJustify = Qt::AlignJustify
    };
    Q_ENUM(HAlignment)

    enum VAlignment {
        AlignTop = Qt::AlignTop,
        AlignBottom = Qt::AlignBottom,
        AlignVCenter = Qt::AlignVCenter
    };
    Q_ENUM(VAlignment)

    enum TextFormat {
        PlainText = Qt::PlainText,
        RichText = Qt::RichText,
        AutoText = Qt::AutoText,
        MarkdownText = Qt::MarkdownText
    };
    Q_ENUM(TextFormat)

    enum WrapMode { NoWrap = QTextOption::NoWrap,
                    WordWrap = QTextOption::WordWrap,
                    WrapAnywhere = QTextOption::WrapAnywhere,
                    WrapAtWordBoundaryOrAnywhere = QTextOption::WrapAtWordBoundaryOrAnywhere, // COMPAT
                    Wrap = QTextOption::WrapAtWordBoundaryOrAnywhere
                  };
    Q_ENUM(WrapMode)

    enum SelectionMode {
        SelectCharacters,
        SelectWords
    };
    Q_ENUM(SelectionMode)

    enum RenderType { QtRendering,
                      NativeRendering
                    };
    Q_ENUM(RenderType)

    QString text() const;
    void setText(const QString &);

    Q_REVISION(2, 7) QString preeditText() const;

    TextFormat textFormat() const;
    void setTextFormat(TextFormat format);

    QFont font() const;
    void setFont(const QFont &font);

    QColor color() const;
    void setColor(const QColor &c);

    QColor selectionColor() const;
    void setSelectionColor(const QColor &c);

    QColor selectedTextColor() const;
    void setSelectedTextColor(const QColor &c);

    HAlignment hAlign() const;
    void setHAlign(HAlignment align);
    void resetHAlign();
    HAlignment effectiveHAlign() const;

    VAlignment vAlign() const;
    void setVAlign(VAlignment align);

    WrapMode wrapMode() const;
    void setWrapMode(WrapMode w);

    int lineCount() const;

    int length() const;

    bool isCursorVisible() const;
    void setCursorVisible(bool on);

    int cursorPosition() const;
    void setCursorPosition(int pos);

    QQmlComponent* cursorDelegate() const;
    void setCursorDelegate(QQmlComponent*);

    bool overwriteMode() const;
    void setOverwriteMode(bool overwrite);

    int selectionStart() const;
    int selectionEnd() const;

    QString selectedText() const;

    bool focusOnPress() const;
    void setFocusOnPress(bool on);

    bool persistentSelection() const;
    void setPersistentSelection(bool on);

    qreal textMargin() const;
    void setTextMargin(qreal margin);

    Qt::InputMethodHints inputMethodHints() const;
    void setInputMethodHints(Qt::InputMethodHints hints);

    bool selectByKeyboard() const;
    void setSelectByKeyboard(bool);

    bool selectByMouse() const;
    void setSelectByMouse(bool);

    SelectionMode mouseSelectionMode() const;
    void setMouseSelectionMode(SelectionMode mode);

    bool canPaste() const;

    bool canUndo() const;
    bool canRedo() const;

    void componentComplete() override;

    /* FROM EDIT */
    void setReadOnly(bool);
    bool isReadOnly() const;

    QRectF cursorRectangle() const;

#if QT_CONFIG(im)
    QVariant inputMethodQuery(Qt::InputMethodQuery property) const override;
    Q_REVISION(2, 4) Q_INVOKABLE QVariant inputMethodQuery(Qt::InputMethodQuery query, QVariant argument) const;
#endif

    qreal contentWidth() const;
    qreal contentHeight() const;

    QUrl baseUrl() const;
    void setBaseUrl(const QUrl &url);
    void resetBaseUrl();

    Q_INVOKABLE QRectF positionToRectangle(int) const;
    Q_INVOKABLE int positionAt(qreal x, qreal y) const;
    Q_INVOKABLE void moveCursorSelection(int pos);
    Q_INVOKABLE void moveCursorSelection(int pos, SelectionMode mode);

    QRectF boundingRect() const override;
    QRectF clipRect() const override;

    bool isInputMethodComposing() const;

    RenderType renderType() const;
    void setRenderType(RenderType renderType);

    Q_INVOKABLE QString getText(int start, int end) const;
    Q_INVOKABLE QString getFormattedText(int start, int end) const;

    QQuickTextDocument *textDocument();

    QString hoveredLink() const;

    Q_REVISION(2, 3) Q_INVOKABLE QString linkAt(qreal x, qreal y) const;

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

    int tabStopDistance() const;
    void setTabStopDistance(qreal distance);

    void invalidate() override;

Q_SIGNALS:
    void textChanged();
    Q_REVISION(2, 7) void preeditTextChanged();
    void contentSizeChanged();
    void cursorPositionChanged();
    void cursorRectangleChanged();
    void selectionStartChanged();
    void selectionEndChanged();
    void selectedTextChanged();
    void colorChanged(const QColor &color);
    void selectionColorChanged(const QColor &color);
    void selectedTextColorChanged(const QColor &color);
    void fontChanged(const QFont &font);
    void horizontalAlignmentChanged(QQuickTextEdit::HAlignment alignment);
    void verticalAlignmentChanged(QQuickTextEdit::VAlignment alignment);
    void wrapModeChanged();
    void lineCountChanged();
    void textFormatChanged(QQuickTextEdit::TextFormat textFormat);
    void readOnlyChanged(bool isReadOnly);
    void cursorVisibleChanged(bool isCursorVisible);
    void cursorDelegateChanged();
    void overwriteModeChanged(bool overwriteMode);
    void activeFocusOnPressChanged(bool activeFocusOnPressed);
    void persistentSelectionChanged(bool isPersistentSelection);
    void textMarginChanged(qreal textMargin);
    Q_REVISION(2, 1) void selectByKeyboardChanged(bool selectByKeyboard);
    void selectByMouseChanged(bool selectByMouse);
    void mouseSelectionModeChanged(QQuickTextEdit::SelectionMode mode);
    void linkActivated(const QString &link);
    Q_REVISION(2, 2) void linkHovered(const QString &link);
    void canPasteChanged();
    void canUndoChanged();
    void canRedoChanged();
    void inputMethodComposingChanged();
    void effectiveHorizontalAlignmentChanged();
    void baseUrlChanged();
    void inputMethodHintsChanged();
    void renderTypeChanged();
    Q_REVISION(2, 6) void editingFinished();
    Q_REVISION(2, 6) void paddingChanged();
    Q_REVISION(2, 6) void topPaddingChanged();
    Q_REVISION(2, 6) void leftPaddingChanged();
    Q_REVISION(2, 6) void rightPaddingChanged();
    Q_REVISION(2, 6) void bottomPaddingChanged();
    Q_REVISION(2, 10) void tabStopDistanceChanged(qreal distance);

public Q_SLOTS:
    void selectAll();
    void selectWord();
    void select(int start, int end);
    void deselect();
    bool isRightToLeft(int start, int end);
#if QT_CONFIG(clipboard)
    void cut();
    void copy();
    void paste();
#endif
    void undo();
    void redo();
    void insert(int position, const QString &text);
    void remove(int start, int end);
    Q_REVISION(2, 2) void append(const QString &text);
    Q_REVISION(2, 7) void clear();

private Q_SLOTS:
    void q_invalidate();
    void q_textChanged();
    void q_contentsChange(int, int, int);
    void updateSelection();
    void moveCursorDelegate();
    void createCursor();
    void q_canPasteChanged();
    void updateWholeDocument();
    void invalidateBlock(const QTextBlock &block);
    void updateCursor();
    void q_linkHovered(const QString &link);
    void q_markerHovered(bool hovered);
    void q_updateAlignment();
    void updateSize();
    void triggerPreprocess();

private:
    void markDirtyNodesForRange(int start, int end, int charDelta);
    void updateTotalLines();
    void invalidateFontCaches();

protected:
    QQuickTextEdit(QQuickTextEditPrivate &dd, QQuickItem *parent = nullptr);
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    void setOldSelectionDefault();
#endif

    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    bool event(QEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

    // mouse filter?
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
#if QT_CONFIG(im)
    void inputMethodEvent(QInputMethodEvent *e) override;
#endif
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;
    void updatePolish() override;

    friend class QQuickTextUtil;
    friend class QQuickTextDocument;

private:
    Q_DISABLE_COPY(QQuickTextEdit)
    Q_DECLARE_PRIVATE(QQuickTextEdit)
};

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
class QQuickPre64TextEdit : public QQuickTextEdit {
    Q_OBJECT
    QML_NAMED_ELEMENT(TextEdit)
    QML_ADDED_IN_VERSION(2, 0)
    QML_REMOVED_IN_VERSION(6, 4)
public:
    QQuickPre64TextEdit(QQuickItem *parent = nullptr);
};
#endif

Q_DECLARE_MIXED_ENUM_OPERATORS_SYMMETRIC(int, QQuickTextEdit::HAlignment, QQuickTextEdit::VAlignment)

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickTextEdit)

#endif // QQUICKTEXTEDIT_P_H
