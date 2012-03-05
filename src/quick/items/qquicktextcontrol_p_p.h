/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTEXTCONTROL_P_P_H
#define QTEXTCONTROL_P_P_H

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

    void init(Qt::TextFormat format = Qt::RichText, const QString &text = QString(),
              QTextDocument *document = 0);
    void setContent(Qt::TextFormat format = Qt::RichText, const QString &text = QString(),
                    QTextDocument *document = 0);

    void paste(const QMimeData *source);

    void setCursorPosition(const QPointF &pos);
    void setCursorPosition(int pos, QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

    void repaintCursor();
    inline void repaintSelection()
    { repaintOldAndNewSelection(QTextCursor()); }
    void repaintOldAndNewSelection(const QTextCursor &oldSelection);

    void selectionChanged(bool forceEmitSelectionChanged = false);

    void _q_updateCurrentCharFormatAndSelection();

#ifndef QT_NO_CLIPBOARD
    void setClipboardSelection();
#endif

    void _q_emitCursorPosChanged(const QTextCursor &someCursor);

    void setBlinkingCursorEnabled(bool enable);

    void extendWordwiseSelection(int suggestedNewPosition, qreal mouseXPosition);
    void extendBlockwiseSelection(int suggestedNewPosition);

    void _q_deleteSelected();

    void _q_setCursorAfterUndoRedo(int undoPosition, int charsAdded, int charsRemoved);

    QRectF cursorRectPlusUnicodeDirectionMarkers(const QTextCursor &cursor) const;
    QRectF rectForPosition(int position) const;
    QRectF selectionRect(const QTextCursor &cursor) const;
    inline QRectF selectionRect() const
    { return selectionRect(this->cursor); }

    QString anchorForCursor(const QTextCursor &anchor) const;

    void keyPressEvent(QKeyEvent *e);
    void mousePressEvent(QMouseEvent *event, const QPointF &pos);
    void mouseMoveEvent(QMouseEvent *event, const QPointF &pos);
    void mouseReleaseEvent(QMouseEvent *event, const QPointF &pos);
    void mouseDoubleClickEvent(QMouseEvent *event, const QPointF &pos);
    bool sendMouseEventToInputContext(QMouseEvent *event, const QPointF &pos);
    void focusEvent(QFocusEvent *e);
    void inputMethodEvent(QInputMethodEvent *);

    void activateLinkUnderCursor(QString href = QString());

    bool isPreediting() const;
    void commitPreedit();

    QPointF trippleClickPoint;
    QPointF mousePressPos;

    QTextCharFormat lastCharFormat;

    QTextDocument *doc;
    QTextCursor cursor;
    QTextCursor selectedWordOnDoubleClick;
    QTextCursor selectedBlockOnTrippleClick;
    QString tentativeCommit;
    QString highlightedAnchor; // Anchor below cursor
    QString anchorOnMousePress;
    QString linkToCopy;

    QBasicTimer cursorBlinkTimer;
    QBasicTimer trippleClickTimer;

    int preeditCursor;

    Qt::TextInteractionFlags interactionFlags;

    bool cursorOn : 1;
    bool cursorIsFocusIndicator : 1;
    bool mousePressed : 1;
    bool lastSelectionState : 1;
    bool ignoreAutomaticScrollbarAdjustement : 1;
    bool overwriteMode : 1;
    bool acceptRichText : 1;
    bool hideCursor : 1; // used to hide the cursor in the preedit area
    bool hasFocus : 1;
    bool isEnabled : 1;
    bool hadSelectionOnMousePress : 1;
    bool wordSelectionEnabled : 1;

    void _q_copyLink();
    void _q_updateBlock(const QTextBlock &);
    void _q_documentLayoutChanged();
};

QT_END_NAMESPACE

#endif // QQuickTextControl_P_H
