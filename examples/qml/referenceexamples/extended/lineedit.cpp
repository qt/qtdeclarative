// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "lineedit.h"
#include <qqml.h>

LineEditExtension::LineEditExtension(QObject *object)
: QObject(object), m_lineedit(qobject_cast<QLineEdit *>(object))
{
}

int LineEditExtension::leftMargin() const
{
    return m_lineedit->textMargins().left();
}

void LineEditExtension::setLeftMargin(int l)
{
    QMargins m = m_lineedit->textMargins();
    m.setLeft(l);
    m_lineedit->setTextMargins(m);
}

int LineEditExtension::rightMargin() const
{
    return m_lineedit->textMargins().right();
}

void LineEditExtension::setRightMargin(int r)
{
    QMargins m = m_lineedit->textMargins();
    m.setRight(r);
    m_lineedit->setTextMargins(m);
}

int LineEditExtension::topMargin() const
{
    return m_lineedit->textMargins().top();
}

void LineEditExtension::setTopMargin(int t)
{
    QMargins m = m_lineedit->textMargins();
    m.setTop(t);
    m_lineedit->setTextMargins(m);
}

int LineEditExtension::bottomMargin() const
{
    return m_lineedit->textMargins().bottom();
}

void LineEditExtension::setBottomMargin(int b)
{
    QMargins m = m_lineedit->textMargins();
    m.setBottom(b);
    m_lineedit->setTextMargins(m);
}


