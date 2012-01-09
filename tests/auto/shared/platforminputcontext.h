/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <qplatforminputcontext_qpa.h>
#include <QtGui/QInputPanel>

class PlatformInputContext : public QPlatformInputContext
{
public:
    PlatformInputContext()
        : m_visible(false), m_action(QInputPanel::Click), m_cursorPosition(0),
          m_invokeActionCallCount(0), m_showInputPanelCallCount(0), m_hideInputPanelCallCount(0),
          m_updateCallCount(0), m_direction(Qt::LeftToRight)
    {
    }

    virtual void showInputPanel()
    {
        m_visible = true;
        m_showInputPanelCallCount++;
    }
    virtual void hideInputPanel()
    {
        m_visible = false;
        m_hideInputPanelCallCount++;
    }
    virtual bool isInputPanelVisible() const
    {
        return m_visible;
    }
    virtual void invokeAction(QInputPanel::Action action, int cursorPosition)
    {
        m_invokeActionCallCount++;
        m_action = action;
        m_cursorPosition = cursorPosition;
    }
    virtual void update(Qt::InputMethodQueries)
    {
        m_updateCallCount++;
    }

    virtual QLocale locale() const
    {
        if (m_direction == Qt::RightToLeft)
            return QLocale(QLocale::Arabic);
        else
            return QLocale(QLocale::English);
    }

    virtual Qt::LayoutDirection inputDirection() const
    {
        return m_direction;
    }

    void setInputDirection(Qt::LayoutDirection direction) {
        m_direction = direction;
        emitLocaleChanged();
        emitInputDirectionChanged(inputDirection());
    }

    void clear() {
        m_cursorPosition = 0;
        m_invokeActionCallCount = 0;
        m_visible = false;
        m_showInputPanelCallCount = 0;
        m_hideInputPanelCallCount = 0;
        m_updateCallCount = 0;
    }

    bool m_visible;
    QInputPanel::Action m_action;
    int m_cursorPosition;
    int m_invokeActionCallCount;
    int m_showInputPanelCallCount;
    int m_hideInputPanelCallCount;
    int m_updateCallCount;
    Qt::LayoutDirection m_direction;
};
