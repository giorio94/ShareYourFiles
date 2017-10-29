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

#include "syfitprotocol.hpp"
#include "Common/common.hpp"

#include <Logger.h>

#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtEndian>

// Register UserIcon to the qt meta type system
static MetaTypeRegistration<UserIcon> iconRegisterer("UserIcon");


///
/// The constructor initializes the internal fields of the instance
/// but does not actually start waiting for incoming requests.
///
/// \see start(quint32)
///
SyfitProtocolServer::SyfitProtocolServer(const QByteArray &iconData,
                                         QObject *parent)
        : QObject(parent),
          m_server(new QTcpServer(this)),
          m_iconLength(qToLittleEndian(iconData.length())),
          m_iconData(new QByteArray(iconData))
{
    LOG_ASSERT_X(!iconData.isEmpty(),
                 "SyfitProtcolServer: trying to use an empty icon");

    // Connect to the handler for a new request
    connect(m_server, &QTcpServer::newConnection, this,
            &SyfitProtocolServer::serveClient);

    // Connect to the handler in case of error
    connect(m_server, &QTcpServer::acceptError, this, [this]() {
        LOG_WARNING() << "SyfitProtocolServer: error while accepting a"
                         " new connection"
                      << m_server->errorString();
    });
}

///
/// The server is destroyed automatically since it is a children of the
/// current instance.
///
SyfitProtocolServer::~SyfitProtocolServer()
{
    LOG_INFO() << "SyfitProtocolServer: stopped";
}

///
/// The socket is configured in listening state: the IPv4 address chosen for
/// listening is specified by the parameter, while the choice of the port is
/// left to the OS (returned as parameter). In case of failure, the special
/// port value SyfitProtocolServer::INVALID_PORT is returned.
///
quint16 SyfitProtocolServer::start(quint32 ipv4Address)
{
    LOG_ASSERT_X(!m_server->isListening(),
                 "SyfitProtocolServer: already started");

    // Start listening for requests
    if (!m_server->listen(QHostAddress(ipv4Address))) {
        LOG_ERROR() << "SfyitProtocolServer: impossible to start the server";
        return SyfitProtocolServer::INVALID_PORT;
    }

    LOG_INFO() << "SyfitProtocolServer: started listening on"
               << qUtf8Printable(m_server->serverAddress().toString()) << "@"
               << m_server->serverPort();

    return m_server->serverPort();
}

///
/// This method is in charge of serving the clients of the SYFIT protocol:
/// the next pending connection is selected, the data is written according
/// to the format, and then the connection is closed.
///
void SyfitProtocolServer::serveClient()
{
    // Repeat until there are pending connections
    while (m_server->hasPendingConnections()) {

        // Get the connected socket
        QPointer<QTcpSocket> connectedSocket =
            m_server->nextPendingConnection();
        connect(connectedSocket, &QAbstractSocket::disconnected,
                connectedSocket, &QObject::deleteLater);

        // Send the length of the data
        if (connectedSocket->write(
                reinterpret_cast<const char *>(&m_iconLength),
                sizeof(m_iconLength)) != sizeof(m_iconLength)) {

            LOG_WARNING() << "SyfitProtocolServer: failed sending data:"
                          << connectedSocket->errorString();
            continue;
        }

        // Send the actual data
        if (connectedSocket->write(*m_iconData) != m_iconData->size()) {
            LOG_WARNING() << "SyfitProtocolServer: failed sending data:"
                          << connectedSocket->errorString();
            continue;
        }

        LOG_INFO() << "SyfitProtocolServer: icon sent to"
                   << qUtf8Printable(connectedSocket->peerAddress().toString())
                   << "@" << connectedSocket->peerPort();

        // Close the connected socket
        connectedSocket->disconnectFromHost();
    }
}


/******************************************************************************/


///
/// The instance is created by initializing the fields and connecting the
/// slots in charge of handling the various events during the transfer.
///
SyfitProtocolClient::SyfitProtocolClient(const QString &confPath,
                                         const QString &uuid, QObject *parent)
        : QObject(parent),
          m_confPath(confPath),
          m_uuid(uuid),
          m_socket(new QTcpSocket(this)),
          m_expectedHash(new QByteArray()),
          m_serverAddress(0),
          m_serverPort(0),
          m_buffer(new QByteArray()),
          m_timerTimeout(new QTimer(this))
{
    // Connect to the handler for bytes ready to be read
    connect(m_socket, &QTcpSocket::readyRead, this,
            &SyfitProtocolClient::readData);

    // Connect to the handler in case of error
    connect(m_socket,
            static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(
                &QTcpSocket::error),
            this, [this]() { manageError(m_socket->errorString()); });

    // Set up the timer to interrupt too long connections
    m_timerTimeout->setSingleShot(true);
    connect(m_timerTimeout, &QTimer::timeout, this,
            [this]() { manageError("timeout"); });
}

///
/// The method is in charge of receiving as parameters the data advertised
/// through the SyfdDatagram regarding the icon and to request that icon
/// to the SyfitProtocolServer. More in detail, the new parameters are
/// compared to the cached ones: in case of equality the method just returns,
/// while in case of difference they are updated and the connection is
/// (re)started.
///
void SyfitProtocolClient::updateAndStart(quint32 serverAddress,
                                         quint16 serverPort,
                                         const QByteArray &iconHash)
{
    // If some parameter changed
    if (m_serverAddress != serverAddress || m_serverPort != serverPort ||
        *m_expectedHash != iconHash) {

        // Save the parameters
        m_serverAddress = serverAddress;
        m_serverPort = serverPort;
        m_expectedHash->clear();
        m_expectedHash->append(iconHash);

        // Reset the retry time
        m_retryTime = SyfitProtocolClient::INITIAL_RETRY_TIME;

        // Stop the previous connection if running
        abortConnection();

        if (m_serverPort != SyfitProtocolServer::INVALID_PORT) {
            // Start the connection
            startConnection();
        } else {
            // The server has some problems
            LOG_WARNING()
                << "SyfitProtocolClient: impossible to start icon request "
                   "(the server has some problems)"
                << qUtf8Printable(m_uuid);
            return;
        }
    }
}

///
/// This method is in charge of actually reading the data received from
/// the server. Initially, if the expected icon length has not yet been set,
/// the value is read and converted to host byte order; all the remaining
/// bytes are then appended to the buffer until the expected length is
/// reached. When the buffer is full, the data is used to try to build
/// an UserIcon instance and, in case of success, the finished() signal is
/// emitted. If an error occurs, on the other hand, the manageError() function
/// is invoked.
///
void SyfitProtocolClient::readData()
{
    // Expected length still not read
    if (m_expectedLength == 0) {

        // Check if the length is already available
        if (m_socket->bytesAvailable() <
            static_cast<qint64>(sizeof(m_expectedLength))) {
            // No: retry later
            return;
        }

        // Yes: read it from socket
        m_socket->read(reinterpret_cast<char *>(&m_expectedLength),
                       sizeof(m_expectedLength));
        // Convert to host byte order
        m_expectedLength = qFromLittleEndian(m_expectedLength);

        // Check if the obtained value is feasible
        if (m_expectedLength > UserIcon::ICON_MAX_SIZE_BYTES) {
            manageError("too big icon detected");
            return;
        }

        m_buffer->reserve(static_cast<int>(m_expectedLength));
    }

    // Read available data and append it to the buffer
    quint32 remaining =
        m_expectedLength - static_cast<quint32>(m_buffer->size());
    m_buffer->append(m_socket->read(remaining));

    // If all data has been read
    if (static_cast<quint32>(m_buffer->size()) == m_expectedLength) {

        // Close the connection and stop the timeout timer
        m_timerTimeout->stop();
        m_socket->disconnectFromHost();

        // Try to build an user icon instance
        UserIcon icon(m_confPath, m_uuid, *m_buffer, *m_expectedHash);

        // In case of success, emit the finished signal
        if (icon.set()) {
            LOG_INFO() << "SyfitProtocolClient: icon request completed"
                       << qUtf8Printable(m_uuid);
            emit finished(icon);
        }

        // Otherwise, manage the error
        else {
            manageError("failed creating the UserIcon instance");
        }
    }
}

///
/// The function appends the error message to the log, aborts the
/// connection and then starts a timer in charge of retrying the transfer
/// after retry time milliseconds (doubled after any error).
///
void SyfitProtocolClient::manageError(const QString &message)
{
    LOG_WARNING() << "SyfitProtocolClient:" << message
                  << "while requesting icon of" << qUtf8Printable(m_uuid);

    // Abort the faulty connection
    abortConnection();

    // Retry to request the icon after retryTime milliseconds
    QTimer::singleShot(m_retryTime, this, [this]() { startConnection(); });
    m_retryTime *= 2;
}

///
/// The function attempts to start a new connection and then starts
/// the timeout timer in order to prevent too long (e.g. due to server
/// errors) connections.
///
void SyfitProtocolClient::startConnection()
{
    // Clear the buffer
    m_expectedLength = 0;
    m_buffer->clear();

    LOG_INFO() << "SyfitProtocolClient: starting icon request"
               << qUtf8Printable(m_uuid);

    // Start the timeout timer
    m_timerTimeout->start(SyfitProtocolClient::TIMEOUT);

    // Connect to the server
    m_socket->connectToHost(QHostAddress(m_serverAddress), m_serverPort,
                            QTcpSocket::ReadOnly);
}

///
/// The function aborts the current connection and stops the timeout timer.
///
void SyfitProtocolClient::abortConnection()
{
    // Stop the timeout timer
    m_timerTimeout->stop();

    // Abort the connection
    m_socket->abort();
}
