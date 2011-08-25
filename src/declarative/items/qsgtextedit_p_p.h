// Commit: 27e4302b7f45f22180693d26747f419177c81e27
/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGTEXTEDIT_P_P_H
#define QSGTEXTEDIT_P_P_H

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

#include "qsgtextedit_p.h"
#include "qsgimplicitsizeitem_p_p.h"

#include <QtDeclarative/qdeclarative.h>

QT_BEGIN_NAMESPACE
class QTextLayout;
class QTextDocument;
class QTextControl;
class QSGTextEditPrivate : public QSGImplicitSizeItemPrivate
{
    Q_DECLARE_PUBLIC(QSGTextEdit)

public:
    QSGTextEditPrivate()
      : color("black"), hAlign(QSGTextEdit::AlignLeft), vAlign(QSGTextEdit::AlignTop),
      documentDirty(true), dirty(false), richText(false), cursorVisible(false), focusOnPress(true),
      showInputPanelOnFocus(true), clickCausedFocus(false), persistentSelection(true),
      requireImplicitWidth(false), selectByMouse(false), canPaste(false),
      hAlignImplicit(true), rightToLeftText(false), isComplexRichText(false),
      textMargin(0.0), lastSelectionStart(0), lastSelectionEnd(0), cursorComponent(0), cursor(0),
      format(QSGTextEdit::AutoText), document(0), wrapMode(QSGTextEdit::NoWrap),
      mouseSelectionMode(QSGTextEdit::SelectCharacters),
      yoff(0), nodeType(NodeIsNull), texture(0)
    {
#ifdef Q_OS_SYMBIAN
        if (QSysInfo::symbianVersion() == QSysInfo::SV_SF_1 || QSysInfo::symbianVersion() == QSysInfo::SV_SF_3) {
            showInputPanelOnFocus = false;
        }
#endif
    }

    void init();

    void updateDefaultTextOption();
    void relayoutDocument();
    void updateSelection();
    bool determineHorizontalAlignment();
    bool setHAlign(QSGTextEdit::HAlignment, bool forceAlign = false);
    void mirrorChange();
    qreal getImplicitWidth() const;

    QString text;
    QFont font;
    QFont sourceFont;
    QColor  color;
    QColor  selectionColor;
    QColor  selectedTextColor;
    QString style;
    QColor  styleColor;
    QSGTextEdit::HAlignment hAlign;
    QSGTextEdit::VAlignment vAlign;

    bool documentDirty : 1;
    bool dirty : 1;
    bool richText : 1;
    bool cursorVisible : 1;
    bool focusOnPress : 1;
    bool showInputPanelOnFocus : 1;
    bool clickCausedFocus : 1;
    bool persistentSelection : 1;
    bool requireImplicitWidth:1;
    bool selectByMouse:1;
    bool canPaste:1;
    bool hAlignImplicit:1;
    bool rightToLeftText:1;
    bool isComplexRichText:1;

    qreal textMargin;
    int lastSelectionStart;
    int lastSelectionEnd;
    QDeclarativeComponent* cursorComponent;
    QSGItem* cursor;
    QSGTextEdit::TextFormat format;
    QTextDocument *document;
    QTextControl *control;
    QSGTextEdit::WrapMode wrapMode;
    QSGTextEdit::SelectionMode mouseSelectionMode;
    int lineCount;
    int yoff;
    QSize paintedSize;

    enum NodeType {
        NodeIsNull,
        NodeIsTexture,
        NodeIsText
    };
    NodeType nodeType;
    QSGTexture *texture;
    QPixmap pixmapCache;
};

QT_END_NAMESPACE

#endif // QSGTEXTEDIT_P_P_H
