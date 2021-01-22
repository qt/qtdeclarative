/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQUICKTEXTCONTROL_P_P_H
#define QQUICKTEXTCONTROL_P_P_H

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

#include "QtGui/qtextdocumentfragment.h"
#include "QtGui/qtextcursor.h"
#include "QtGui/qtextformat.h"
#include "QtGui/qtextobject.h"
#include "QtGui/qabstracttextdocumentlayout.h"
#include "QtCore/qbasictimer.h"
#include "QtCore/qpointer.h"
#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

class QMimeData;
class QAbstractScrollArea;

class QQuickTextControlPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickTextControl)
public:
    QQuickTextControlPrivate();

    bool cursorMoveKeyEvent(QKeyEvent *e);

    void updateCurrentCharFormat();

    void setContent(Qt::TextFormat format, const QString &text);

    void paste(const QMimeData *source);

    void setCursorPosition(const QPointF &pos);
    void setCursorPosition(int pos, QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

    void repaintCursor();
    inline void repaintSelection()
    { repaintOldAndNewSelection(QTextCursor()); }
    void repaintOldAndNewSelection(const QTextCursor &oldSelection);

    void selectionChanged(bool forceEmitSelectionChanged = false);

    void _q_updateCurrentCharFormatAndSelection();

#if QT_CONFIG(clipboard)
    void setClipboardSelection();
#endif

    void _q_updateCursorPosChanged(const QTextCursor &someCursor);

    void setBlinkingCursorEnabled(bool enable);
    void updateCursorFlashTime();

    void extendWordwiseSelection(int suggestedNewPosition, qreal mouseXPosition);
    void extendBlockwiseSelection(int suggestedNewPosition);

    void _q_setCursorAfterUndoRedo(int undoPosition, int charsAdded, int charsRemoved);

    QRectF rectForPosition(int position) const;

    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    void mousePressEvent(QMouseEvent *event, const QPointF &pos);
    void mouseMoveEvent(QMouseEvent *event, const QPointF &pos);
    void mouseReleaseEvent(QMouseEvent *event, const QPointF &pos);
    void mouseDoubleClickEvent(QMouseEvent *event, const QPointF &pos);
    bool sendMouseEventToInputContext(QMouseEvent *event, const QPointF &pos);
    void focusEvent(QFocusEvent *e);
#if QT_CONFIG(im)
    void inputMethodEvent(QInputMethodEvent *);
#endif
    void hoverEvent(QHoverEvent *e, const QPointF &pos);

    void activateLinkUnderCursor(QString href = QString());

#if QT_CONFIG(im)
    bool isPreediting() const;
    void commitPreedit();
    void cancelPreedit();
#endif

    QPointF tripleClickPoint;
    QPointF mousePressPos;

    QTextCharFormat lastCharFormat;

    QTextDocument *doc;
    QTextCursor cursor;
    QTextCursor selectedWordOnDoubleClick;
    QTextCursor selectedBlockOnTripleClick;
    QString anchorOnMousePress;
    QString linkToCopy;
    QString hoveredLink;
    QTextBlock blockWithMarkerUnderMousePress;

    QBasicTimer cursorBlinkTimer;
    ulong timestampAtLastDoubleClick = 0;   // will only be set at a double click

#if QT_CONFIG(im)
    int preeditCursor;
#endif

    Qt::TextInteractionFlags interactionFlags;

    bool cursorOn : 1;
    bool cursorIsFocusIndicator : 1;
    bool mousePressed : 1;
    bool lastSelectionState : 1;
    bool ignoreAutomaticScrollbarAdjustement : 1;
    bool overwriteMode : 1;
    bool acceptRichText : 1;
    bool cursorVisible : 1; // used to hide the cursor in the preedit area
    bool cursorBlinkingEnabled : 1;
    bool hasFocus : 1;
    bool hadSelectionOnMousePress : 1;
    bool wordSelectionEnabled : 1;
    bool hasImState : 1;
    bool cursorRectangleChanged : 1;
    bool hoveredMarker: 1;

    int lastSelectionStart;
    int lastSelectionEnd;

    void _q_copyLink();
};

QT_END_NAMESPACE

#endif // QQuickTextControl_P_H
