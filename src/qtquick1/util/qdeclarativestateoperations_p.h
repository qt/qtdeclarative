/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVESTATEOPERATIONS_H
#define QDECLARATIVESTATEOPERATIONS_H

#include "QtQuick1/private/qdeclarativestate_p.h"

#include <QtQuick1/qdeclarativeitem.h>
#include <QtQuick1/private/qdeclarativeanchors_p.h>
#include <QtDeclarative/qdeclarativescriptstring.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QDeclarative1ParentChangePrivate;
class Q_AUTOTEST_EXPORT QDeclarative1ParentChange : public QDeclarative1StateOperation, public QDeclarative1ActionEvent
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1ParentChange)

    Q_PROPERTY(QDeclarativeItem *target READ object WRITE setObject)
    Q_PROPERTY(QDeclarativeItem *parent READ parent WRITE setParent)
    Q_PROPERTY(QDeclarativeScriptString x READ x WRITE setX)
    Q_PROPERTY(QDeclarativeScriptString y READ y WRITE setY)
    Q_PROPERTY(QDeclarativeScriptString width READ width WRITE setWidth)
    Q_PROPERTY(QDeclarativeScriptString height READ height WRITE setHeight)
    Q_PROPERTY(QDeclarativeScriptString scale READ scale WRITE setScale)
    Q_PROPERTY(QDeclarativeScriptString rotation READ rotation WRITE setRotation)
public:
    QDeclarative1ParentChange(QObject *parent=0);
    ~QDeclarative1ParentChange();

    QDeclarativeItem *object() const;
    void setObject(QDeclarativeItem *);

    QDeclarativeItem *parent() const;
    void setParent(QDeclarativeItem *);

    QDeclarativeItem *originalParent() const;

    QDeclarativeScriptString x() const;
    void setX(QDeclarativeScriptString x);
    bool xIsSet() const;

    QDeclarativeScriptString y() const;
    void setY(QDeclarativeScriptString y);
    bool yIsSet() const;

    QDeclarativeScriptString width() const;
    void setWidth(QDeclarativeScriptString width);
    bool widthIsSet() const;

    QDeclarativeScriptString height() const;
    void setHeight(QDeclarativeScriptString height);
    bool heightIsSet() const;

    QDeclarativeScriptString scale() const;
    void setScale(QDeclarativeScriptString scale);
    bool scaleIsSet() const;

    QDeclarativeScriptString rotation() const;
    void setRotation(QDeclarativeScriptString rotation);
    bool rotationIsSet() const;

    virtual ActionList actions();

    virtual void saveOriginals();
    //virtual void copyOriginals(QDeclarative1ActionEvent*);
    virtual void execute(Reason reason = ActualChange);
    virtual bool isReversable();
    virtual void reverse(Reason reason = ActualChange);
    virtual QString typeName() const;
    virtual bool override(QDeclarative1ActionEvent*other);
    virtual void rewind();
    virtual void saveCurrentValues();
};

class QDeclarative1StateChangeScriptPrivate;
class Q_AUTOTEST_EXPORT QDeclarative1StateChangeScript : public QDeclarative1StateOperation, public QDeclarative1ActionEvent
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1StateChangeScript)

    Q_PROPERTY(QDeclarativeScriptString script READ script WRITE setScript)
    Q_PROPERTY(QString name READ name WRITE setName)

public:
    QDeclarative1StateChangeScript(QObject *parent=0);
    ~QDeclarative1StateChangeScript();

    virtual ActionList actions();

    virtual QString typeName() const;

    QDeclarativeScriptString script() const;
    void setScript(const QDeclarativeScriptString &);
    
    QString name() const;
    void setName(const QString &);

    virtual void execute(Reason reason = ActualChange);
};

class QDeclarative1AnchorChanges;
class QDeclarative1AnchorSetPrivate;
class Q_AUTOTEST_EXPORT QDeclarative1AnchorSet : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QDeclarativeScriptString left READ left WRITE setLeft RESET resetLeft)
    Q_PROPERTY(QDeclarativeScriptString right READ right WRITE setRight RESET resetRight)
    Q_PROPERTY(QDeclarativeScriptString horizontalCenter READ horizontalCenter WRITE setHorizontalCenter RESET resetHorizontalCenter)
    Q_PROPERTY(QDeclarativeScriptString top READ top WRITE setTop RESET resetTop)
    Q_PROPERTY(QDeclarativeScriptString bottom READ bottom WRITE setBottom RESET resetBottom)
    Q_PROPERTY(QDeclarativeScriptString verticalCenter READ verticalCenter WRITE setVerticalCenter RESET resetVerticalCenter)
    Q_PROPERTY(QDeclarativeScriptString baseline READ baseline WRITE setBaseline RESET resetBaseline)
    //Q_PROPERTY(QDeclarativeItem *fill READ fill WRITE setFill RESET resetFill)
    //Q_PROPERTY(QDeclarativeItem *centerIn READ centerIn WRITE setCenterIn RESET resetCenterIn)

    /*Q_PROPERTY(qreal margins READ margins WRITE setMargins NOTIFY marginsChanged)
    Q_PROPERTY(qreal leftMargin READ leftMargin WRITE setLeftMargin NOTIFY leftMarginChanged)
    Q_PROPERTY(qreal rightMargin READ rightMargin WRITE setRightMargin NOTIFY rightMarginChanged)
    Q_PROPERTY(qreal horizontalCenterOffset READ horizontalCenterOffset WRITE setHorizontalCenterOffset NOTIFY horizontalCenterOffsetChanged())
    Q_PROPERTY(qreal topMargin READ topMargin WRITE setTopMargin NOTIFY topMarginChanged)
    Q_PROPERTY(qreal bottomMargin READ bottomMargin WRITE setBottomMargin NOTIFY bottomMarginChanged)
    Q_PROPERTY(qreal verticalCenterOffset READ verticalCenterOffset WRITE setVerticalCenterOffset NOTIFY verticalCenterOffsetChanged())
    Q_PROPERTY(qreal baselineOffset READ baselineOffset WRITE setBaselineOffset NOTIFY baselineOffsetChanged())*/

public:
    QDeclarative1AnchorSet(QObject *parent=0);
    virtual ~QDeclarative1AnchorSet();

    QDeclarativeScriptString left() const;
    void setLeft(const QDeclarativeScriptString &edge);
    void resetLeft();

    QDeclarativeScriptString right() const;
    void setRight(const QDeclarativeScriptString &edge);
    void resetRight();

    QDeclarativeScriptString horizontalCenter() const;
    void setHorizontalCenter(const QDeclarativeScriptString &edge);
    void resetHorizontalCenter();

    QDeclarativeScriptString top() const;
    void setTop(const QDeclarativeScriptString &edge);
    void resetTop();

    QDeclarativeScriptString bottom() const;
    void setBottom(const QDeclarativeScriptString &edge);
    void resetBottom();

    QDeclarativeScriptString verticalCenter() const;
    void setVerticalCenter(const QDeclarativeScriptString &edge);
    void resetVerticalCenter();

    QDeclarativeScriptString baseline() const;
    void setBaseline(const QDeclarativeScriptString &edge);
    void resetBaseline();

    QDeclarativeItem *fill() const;
    void setFill(QDeclarativeItem *);
    void resetFill();

    QDeclarativeItem *centerIn() const;
    void setCenterIn(QDeclarativeItem *);
    void resetCenterIn();

    /*qreal leftMargin() const;
    void setLeftMargin(qreal);

    qreal rightMargin() const;
    void setRightMargin(qreal);

    qreal horizontalCenterOffset() const;
    void setHorizontalCenterOffset(qreal);

    qreal topMargin() const;
    void setTopMargin(qreal);

    qreal bottomMargin() const;
    void setBottomMargin(qreal);

    qreal margins() const;
    void setMargins(qreal);

    qreal verticalCenterOffset() const;
    void setVerticalCenterOffset(qreal);

    qreal baselineOffset() const;
    void setBaselineOffset(qreal);*/

    QDeclarative1Anchors::Anchors usedAnchors() const;

/*Q_SIGNALS:
    void leftMarginChanged();
    void rightMarginChanged();
    void topMarginChanged();
    void bottomMarginChanged();
    void marginsChanged();
    void verticalCenterOffsetChanged();
    void horizontalCenterOffsetChanged();
    void baselineOffsetChanged();*/

private:
    friend class QDeclarative1AnchorChanges;
    Q_DISABLE_COPY(QDeclarative1AnchorSet)
    Q_DECLARE_PRIVATE(QDeclarative1AnchorSet)
};

class QDeclarative1AnchorChangesPrivate;
class Q_AUTOTEST_EXPORT QDeclarative1AnchorChanges : public QDeclarative1StateOperation, public QDeclarative1ActionEvent
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarative1AnchorChanges)

    Q_PROPERTY(QDeclarativeItem *target READ object WRITE setObject)
    Q_PROPERTY(QDeclarative1AnchorSet *anchors READ anchors CONSTANT)

public:
    QDeclarative1AnchorChanges(QObject *parent=0);
    ~QDeclarative1AnchorChanges();

    virtual ActionList actions();

    QDeclarative1AnchorSet *anchors();

    QDeclarativeItem *object() const;
    void setObject(QDeclarativeItem *);

    virtual void execute(Reason reason = ActualChange);
    virtual bool isReversable();
    virtual void reverse(Reason reason = ActualChange);
    virtual QString typeName() const;
    virtual bool override(QDeclarative1ActionEvent*other);
    virtual bool changesBindings();
    virtual void saveOriginals();
    virtual bool needsCopy() { return true; }
    virtual void copyOriginals(QDeclarative1ActionEvent*);
    virtual void clearBindings();
    virtual void rewind();
    virtual void saveCurrentValues();

    QList<QDeclarative1Action> additionalActions();
    virtual void saveTargetValues();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarative1ParentChange)
QML_DECLARE_TYPE(QDeclarative1StateChangeScript)
QML_DECLARE_TYPE(QDeclarative1AnchorSet)
QML_DECLARE_TYPE(QDeclarative1AnchorChanges)

QT_END_HEADER

#endif // QDECLARATIVESTATEOPERATIONS_H
