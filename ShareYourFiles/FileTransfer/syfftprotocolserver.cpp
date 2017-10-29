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

#include "syfftprotocolserver.hpp"
#include "Common/common.hpp"
#include "syfftprotocolreceiver.hpp"

#include <Logger.h>

#include <QTcpServer>
#include <QTcpSocket>

// Register QSharedPointr<SyfftProtocolReceiver> to the qt meta type system
static MetaTypeRegistration<QSharedPointer<SyfftProtocolReceiver>>
    receiverRegisterer("QSharedPointer<SyfftProtocolReceiver>");

///
/// The instance is created by building a new TCP server and connecting
/// the handlers to manage new connections and possible errors.
///
SyfftProtocolServer::SyfftProtocolServer(const QString &localUuid,
                                         QObject *parent)
        : QObject(parent),
          m_localUuid(localUuid),
          m_server(new QTcpServer(this))
{
    // Connect to the handler for a new request
    connect(m_server, &QTcpServer::newConnection, this,
            &SyfftProtocolServer::newConnection);

    // Connect to the handler in case of error
    connect(m_server, &QTcpServer::acceptError, this, [this]() {
        LOG_WARNING() << "SyfftProtocolServer: error while accepting a"
                         " new connection"
                      << m_server->errorString();
    });
}

///
/// The server is configured in listening state and the necessary
/// handlers are connected. The IPv4 address chosen for listening is
/// specified by the parameter, while the choice of the port is left
/// to the OS (returned as parameter). In case of failure, the special
/// port value SyfitProtocolServer::INVALID_PORT is returned.
///
quint16 SyfftProtocolServer::start(quint32 ipv4Address)
{
    LOG_ASSERT_X(!m_server->isListening(),
                 "SyfftProtocolServer: already started");

    // Start listening for requests
    if (!m_server->listen(QHostAddress(ipv4Address))) {
        LOG_ERROR() << "SyfftProtocolServer: impossible to start the server";
        return SyfftProtocolServer::INVALID_PORT;
    }

    LOG_INFO() << "SyfftProtocolServer: started listening on"
               << qUtf8Printable(m_server->serverAddress().toString()) << "@"
               << m_server->serverPort();

    emit started(m_server->serverPort());
    return m_server->serverPort();
}

///
/// The server is destroyed automatically since it is a children of the
/// current instance.
///
SyfftProtocolServer::~SyfftProtocolServer()
{
    LOG_INFO() << "SyfftProtocolServer: stopped";
    emit stopped();
}

///
/// For each incoming connection, a new instance of SyfftProtocolReceiver is
/// created to complete the connection phase and to receive the actual files.
/// The connectionRequested() signal is then emitted to advertise the new
/// instance.
///
void SyfftProtocolServer::newConnection()
{
    // Repeat until there are pending connections
    while (m_server->hasPendingConnections()) {

        // Create a new SyfftProtocolReceiver instance
        SyfftProtocolReceiver *instance = new SyfftProtocolReceiver(
            m_localUuid, m_server->nextPendingConnection());

        // Connect the slot to abort the connection when the server is
        // terminated
        connect(this, &SyfftProtocolServer::stopped, instance,
                [instance]() { instance->terminateConnection(); });

        // Emit the signal to notify the new connection
        emit connectionRequested(QSharedPointer<SyfftProtocolReceiver>(
            instance, &QObject::deleteLater));
    }
}
