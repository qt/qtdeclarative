#ifndef SEQENCETYPEEXAMPLE_H
#define SEQENCETYPEEXAMPLE_H

#include <QDebug>
#include <QObject>
#include <QtQml>

class SequenceTypeExample : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY (QList<qreal> qrealListProperty READ qrealListProperty WRITE setQrealListProperty
                NOTIFY qrealListPropertyChanged)

public:
    explicit SequenceTypeExample();

    QList<qreal> qrealListProperty() const;
    void setQrealListProperty(const QList<qreal> &list);

signals:
    void qrealListPropertyChanged();

private :
    QList<qreal> m_list;

};

#endif // SEQENCETYPEEXAMPLE_H
