#ifndef QLISTPROVIDER_H
#define QLISTPROVIDER_H

#include <QObject>
#include <QQmlEngine>

class QListProvider : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ListProvider)

public:
    explicit QListProvider(QObject *parent = nullptr) : QObject(parent) { }

    Q_INVOKABLE QList<int> intList() const
    {
        QList<int> list;
        for (int i = 0; i < 3; ++i)
            list.append(i);
        return list;
    }
};

#endif // QLISTPROVIDER_H
