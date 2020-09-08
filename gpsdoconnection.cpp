#include "gpsdoconnection.h"

#include <QTimer>

GpsdoConnection::GpsdoConnection(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, SIGNAL(connected()), this, SIGNAL(connected()));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SIGNAL(socketError(QAbstractSocket::SocketError)));
}

void GpsdoConnection::connectTo(QString server)
{
    m_socket->connectToHost(server, 6502);
}

void GpsdoConnection::readyRead()
{
    char buf[256];
    int size = static_cast<int>(m_socket->readLine(buf, sizeof(buf)));
    if (size > 0) {
        QString data = QString::fromLocal8Bit(buf, size);
        emit dataReady(data);
    }

    // if there's more data, call slot again via event loop
    if (m_socket->canReadLine())
        QTimer::singleShot(0, this, &GpsdoConnection::readyRead);
}

void GpsdoConnection::close()
{
    m_socket->close();
}

void GpsdoConnection::write(QString data)
{
    m_socket->write(data.toLocal8Bit());
}
