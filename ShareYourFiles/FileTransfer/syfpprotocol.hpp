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

#ifndef SYFPPROTOCOL_HPP
#define SYFPPROTOCOL_HPP

#include <QObject>
#include <QPointer>
#include <QStringList>

class QDataStream;
class QLocalServer;
class QLocalSocket;
class QTimer;

///
/// \brief The SyfpProtocolServer class provides the server side implementation
/// of the SYFP Protocol.
///
/// The SYFP (Share Your Files Picker) Protocol is a very simple protocol
/// used to receive the file and directory paths, that are to be scheduled
/// for transfer, from the SYFPicker application (the one executed when the
/// user choses them). It is designed to provide communication between two
/// applications in execution on the same device and, therefore, it exploits
/// the functionalities provided by QLocalSocket and QLocalServer.
///
/// This class provides the server side implementation of the protocol: in
/// particular, it allows starting listening for requests on a specified name:
/// when a new client gets connected, a brand new instance of
/// SyfpProtocolReceiver is created to receive the data and, if the transfer
/// completes correctly, the pathsReceived() signal is finally emitted to
/// advertise the paths of the files and directories to be shared. The server
/// can be terminated by destroying the instance representing it.
///
class SyfpProtocolServer : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs a new instance of the server side SYFP protocol.
    /// \param parent the parent of the current object.
    ///
    explicit SyfpProtocolServer(QObject *parent = Q_NULLPTR);

    ///
    /// \brief Destroys the current instance.
    ///
    ~SyfpProtocolServer();

public slots:
    ///
    /// \brief Starts listening for requests.
    /// \param name the name on which the socket listens.
    /// \return a value indicating whether the operation succeeded or failed.
    ///
    bool start(const QString &name);

signals:
    ///
    /// \brief Signal emitted when a reception terminates correctly.
    /// \param paths the list of paths requested to be shared.
    ///
    void pathsReceived(const QStringList &paths);

private:
    ///
    /// \brief Function executed when a new connection is ready to be accepted.
    ///
    void newConnection();

    /// \brief The socket used for listening.
    QPointer<QLocalServer> m_server;
};

///
/// \brief The SyfpProtocolReceiver class provides the receiver side
/// implementation of the SYFP Protocol.
///
/// When a new connection is established, an instance of this class is created
/// to perform the actual reception according to the SYFP Protocol, which
/// mandates a 32 bits unsigned number representing the number of paths to be
/// received, followed by each of them (a 32 bits unsigned number representing
/// the number of bytes and the actual characters in UTF8 format). The numbers
/// are expected to be transferred in little endian order. If the transfer
/// completes correctly, the finished() signal is emitted, otherwise the error()
/// one is used to signal that something went wrong during the transfer.
///
class SyfpProtocolReceiver : public QObject
{
    Q_OBJECT

public:
    ///
    /// \brief Constructs a new instance of the receiver side SYFP protocol.
    /// \param socket the connected socket to be used for the reception.
    /// \param parent the parent of the current object.
    ///
    explicit SyfpProtocolReceiver(QLocalSocket *socket,
                                  QObject *parent = Q_NULLPTR);

    ///
    /// \brief Destroys the current instance.
    ///
    ~SyfpProtocolReceiver();

signals:
    ///
    /// \brief Signal emitted when the reception terminates correctly.
    /// \param paths the list of paths requested to be shared.
    ///
    void finished(const QStringList &paths);

    ///
    /// \brief Signal emitted when an error occurs.
    ///
    void error();

private:
    ///
    /// \brief Function executed every time some bytes are ready to be read.
    ///
    void readData();

private:
    /// \brief The socket used for the reception.
    QPointer<QLocalSocket> m_socket;
    /// \brief The stream used for the reception.
    QScopedPointer<QDataStream> m_stream;

    quint32 m_pathNumber;   ///< \brief The number of paths to be received.
    QStringList m_pathList; ///< \brief The list of received paths.

    /// \brief The timer used to stop too long connections.
    QPointer<QTimer> m_timerTimeout;

    /// \brief Maximum duration of a connection (in ms).
    static const int TIMEOUT = 5000;
};

#endif // SYFPPROTOCOL_HPP
