/*
 *  Copyright (c) 2017 Marco Iorio (giorio94 at gmail dot com)
 *  This file is part of Share Your Files (SYF).
 *
 *  SYF is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SYF is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with SYF.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "syfpprotocol.hpp"

#include <Logger.h>

#include <QDataStream>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

///
/// The constructor initializes the internal fields of the instance
/// but does not actually start waiting for incoming requests.
///
/// \see start(quint32)
///
SyfpProtocolServer::SyfpProtocolServer(QObject *parent)
        : QObject(parent), m_server(new QLocalServer(this))
{
    // Connect to the handler for a new request
    connect(m_server, &QLocalServer::newConnection, this,
            &SyfpProtocolServer::newConnection);
}

///
/// The server is destroyed automatically since it is a children of the
/// current instance.
///
SyfpProtocolServer::~SyfpProtocolServer()
{
    LOG_INFO() << "SyfpProtocolServer: stopped";
}

///
/// The socket is configured in listening state on the name specified by the
/// parameter.
///
bool SyfpProtocolServer::start(const QString &name)
{
    LOG_ASSERT_X(!m_server->isListening(),
                 "SyfpProtocolServer: already started");

    // Remove the server instance (if present) to avoid problems after a crash
    if (!QLocalServer::removeServer(name)) {
        LOG_ERROR() << "SyfpProtocolServer: failed removing the server -"
                    << qUtf8Printable(m_server->serverName());
    }

    // Start listening for requests
    if (!m_server->listen(name)) {
        LOG_ERROR() << "SyfpProtocolServer: impossible to start the server -"
                    << m_server->serverError();
        return false;
    }

    LOG_INFO() << "SyfpProtocolServer: started listening on"
               << qUtf8Printable(m_server->serverName());

    return true;
}

///
/// This method is executed every time a new connection is ready to be accepted:
/// a brand new SyfpProtocolReceiver instance is created to manage the reception
/// and the handlers are connected to emit the pathsReceived() signal if the
/// data is received correctly and to delete the instance itself.
///
void SyfpProtocolServer::newConnection()
{
    // Repeat until there are pending connections
    while (m_server->hasPendingConnections()) {

        // Get the connected socket and create a new receiver instance
        SyfpProtocolReceiver *receiver =
            new SyfpProtocolReceiver(m_server->nextPendingConnection(), this);

        // Connect the signal handlers
        connect(receiver, &SyfpProtocolReceiver::finished, this,
                &SyfpProtocolServer::pathsReceived);
        connect(receiver, &SyfpProtocolReceiver::finished, receiver,
                &QObject::deleteLater);
        connect(receiver, &SyfpProtocolReceiver::error, receiver,
                &QObject::deleteLater);

        // The receiver instance is automatically deleted if still running when
        // the this instance is terminated
    }
}


/******************************************************************************/


///
/// The instance is created by initializing the fields and connecting the
/// slots in charge of handling the various events during the transfer.
/// The timeout timer is also set up and started to prevent too long
/// connections.
///
SyfpProtocolReceiver::SyfpProtocolReceiver(QLocalSocket *socket,
                                           QObject *parent)
        : QObject(parent),
          m_socket(socket),
          m_stream(new QDataStream(socket)),
          m_pathNumber(0),
          m_timerTimeout(new QTimer(this))
{
    // Take ownership of the socket
    m_socket->setParent(this);

    // Set the stream options
    m_stream->setVersion(QDataStream::Version::Qt_5_0);
    m_stream->setByteOrder(QDataStream::ByteOrder::LittleEndian);

    // Connect to the handler for bytes ready to be read
    connect(m_socket, &QLocalSocket::readyRead, this,
            &SyfpProtocolReceiver::readData);

    // Connect to the handler in case of error
    connect(m_socket,
            static_cast<void (QLocalSocket::*)(QLocalSocket::LocalSocketError)>(
                &QLocalSocket::error),
            this, [this]() {
                LOG_WARNING() << "SyfpProtocolReceiver:"
                              << m_socket->errorString();
                emit error();
            });

    // Set up the timer to interrupt too long connections
    m_timerTimeout->setSingleShot(true);
    connect(m_timerTimeout, &QTimer::timeout, this, [this]() {
        LOG_WARNING() << "SyfpProtocolReceiver: timeout expired";
        emit error();
    });
    m_timerTimeout->start(SyfpProtocolReceiver::TIMEOUT);
}

/// The destruction is automatically performed by the destructors of the
/// members.
SyfpProtocolReceiver::~SyfpProtocolReceiver() {}

///
/// This method is in charge of actually reading the data received from the
/// client. Initially, if the expected number of paths to be received has not
/// yet been set, it is read (32 bits unsigned number); the paths are then
/// read and converted: when the process completes, the finished() signal is
/// emitted to advertise the obtained values.
///
void SyfpProtocolReceiver::readData()
{
    // Continue until data is still available
    while (m_socket->bytesAvailable()) {
        // Number of paths to be received not yet read
        if (m_pathNumber == 0) {

            // Start a new transaction
            m_stream->startTransaction();
            *m_stream >> m_pathNumber;

            // Still missing data
            if (!m_stream->commitTransaction()) {
                m_pathNumber = 0;
                return;
            }
        }

        // Start a new transaction
        m_stream->startTransaction();

        // Read the next path
        QByteArray path;
        *m_stream >> path;

        // Still missing data
        if (!m_stream->commitTransaction()) {
            return;
        }

        // Add the path to the list
        m_pathList << QString::fromUtf8(path);

        // All paths received
        if (static_cast<quint32>(m_pathList.count()) == m_pathNumber) {
            m_timerTimeout->stop();
            m_socket->disconnectFromServer();

            emit finished(m_pathList);
            return;
        }
    }
}
