// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef STATECONTROLLER_H
#define STATECONTROLLER_H

#include <QObject>

class StateController : public QObject
{
    Q_OBJECT

public:
    //  _________
    // |         |<-------------------------------------*
    // |InitState|<-------------------*                 |
    // |_________*----*   _________   |                 |
    //                |  |         |  | setDirty(false) |
    // setDirty(true) *->|NewState *--*                 |
    //                |  |_________*---*                |
    //                |   __________   | changesSaved() |
    //   fileLoaded() *->|          |<-*                | fileClosed()
    //                   |          |-------------------*
    //                *--*OpenState |<------------------*
    //                |  |          *-*                 |
    //   fileLoaded() *->|__________* |                 |
    //                                |                 |
    //                                |   ___________   | changesSaved()
    //                                |  |           |  | or
    //                 setDirty(true) *->|DirtyState |  | setDirty(false)
    //                                   |___________*--*
    enum State {
        InitState,
        NewState,
        OpenState,
        DirtyState,
    };

public:
    explicit StateController(QObject *parent = nullptr);

    State currentState() const { return m_currentState; }
    QString filePath() const { return m_filePath; }

    static StateController* instance();

public slots:
    void setDirty(bool dirty);
    void changesSaved(const QString &filePath = {});
    void fileLoaded(const QString &filePath);
    void fileClosed();

signals:
    void stateChanged(State from, State to);

private:
    void setCurrentState(State state);
    void setFilePath(const QString &filePath = {}) { m_filePath = filePath; }

private:
    State m_currentState = InitState;
    QString m_filePath;
    static inline StateController *s_instance = nullptr;
};

#endif // STATECONTROLLER_H
