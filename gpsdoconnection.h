#ifndef GPSDOCONNECTION_H
#define GPSDOCONNECTION_H

#include <QObject>
#include <QtNetwork/QTcpSocket>

class GpsdoConnection : public QObject
{
    Q_OBJECT
public:
    explicit GpsdoConnection(QObject *parent = nullptr);

    void connectTo(QString);
    void close();
    void write(QString);

signals:
    void dataReady(QString);
    void connected();
    void socketError(QAbstractSocket::SocketError);

public slots:
    void readyRead();

private:
    QTcpSocket *m_socket;
};

#endif // GPSDOCONNECTION_H
