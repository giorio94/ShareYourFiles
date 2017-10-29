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

#ifndef SYFFTPROTOCOLCOMMON_HPP
#define SYFFTPROTOCOLCOMMON_HPP

#include "fileinfo.hpp"

#include <QAtomicInteger>
#include <QDir>
#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QStack>
#include <QVector>

class FileInTransfer;
class TransferInfo;

class QDataStream;
class QElapsedTimer;
class QTcpSocket;
class QTimer;

///
/// \brief The SyfftProtocolCommon class provides an implementation of the
/// common parts of the SYFFT protocol.
///
/// The SYFFT (Share Your Files Files Transfer) Protocol is the network protocol
/// used to actually provide the files sharing services between two instances of
/// Share Your Files operating on different hosts.
///
/// This protocol, which runs on the top of TCP, is based on binary commands
/// (represented by the Command enumeration) to provide the communication
/// between the sending and the receiver instance and mandates the Little Endian
/// representation for the numbers. It is composed by three main phases:
/// connection, transfer and termination; moreover, in case of an unexpected
/// error detected by either one of the instances, the connection is immediately
/// aborted through the ABORT command and resetting the TCP socket.
///
/// The connection phase, which starts immediately after the TCP connection has
/// been correctly established, proceeds as following: the sending instance, the
/// one which connected to the SYFFT Protocol Server, sends the HELLO command
/// followed by its UUID (as a 16 bytes array in Rfc4122 format), to which the
/// receiving side answers using the same format. If everything completes
/// correctly, the sending instance acknowledges the connection with the ACK
/// command; it is then possible to advertise the list of files to be
/// transferred to the receiving side, by sending the SHARE command, followed by
/// the number of files (32 bits unsigned number), their total size (64 bits
/// unsigned number) and an optional message attached by the user (a 32 bits
/// unsigned number representing the number of bytes followed by the actual
/// characters in UTF8 format). The whole list of files is also advertised, by
/// sending the information about every file (according to the FileInfo format,
/// preceded by the ITEM command); the sharing request is then completed by
/// sending again the SHARE command. The request can be either accepted (ACCEPT
/// command) or rejected (REJECT command) by the peer, optionally attaching a
/// textual message (in the same format as before): the instances move then to
/// the next phase accordingly.
///
/// The transfer phase, which starts after the acceptance of the sharing
/// request, is the one performing the actual file transfers: for each file to
/// be transmitted, the protocol mandates the following message exchanges: the
/// sending side verifies if it is possible to read the current file and
/// according to the result sends either the START or the SKIP command to the
/// peer. In the first case, the other instance of the protocol can either
/// ACCEPT or REJECT the current file (in particular it is rejected if it is
/// impossible to write to the destination file or if a file with the same name
/// already exists and the user chose to keep it), while in the latter it can
/// only be rejected. If the file is rejected, both instances move to the next
/// file, otherwise it can finally be sent: the sending side starts reading
/// chunks of the file and delivering them as a CHUNK command, followed by the
/// chunk size (32 bits unsigned number) and the actual data. If, during the
/// reception, an error is detected (e.g it is impossible to write to the
/// destination file), the receiving side can stop the peer through the STOP
/// command: it will then enter in rollback state and stop as soon as possible
/// sending the data; in a similar way, if an error is detected by the sending
/// side, the rollback state is entered. In both cases, the sender dispatches a
/// ROLLBACK command to notify the peer about the error, which is expected to
/// answer with the same command before moving to the next file. In case the
/// transfer completes correctly, and after having verified if it possible to
/// commit it, the COMMIT command is notified to the receiving side of the
/// protocol: in turn, it checks the correct reception of the file and answers
/// either with a COMMIT or a ROLLBACK command. In any case, it is possible to
/// move to the next file and, it the current one was the last, it is possible
/// to enter the termination phase.
///
/// The termination phase, which starts after the rejection or the termination
/// of the sharing session, is composed by a simple CLOSE command exchange:
/// after having sent it, the instances immediately disconnects from the peer.
///
/// Moreover, the protocol provides the possibility to enter in pause mode
/// (either after a request from the user or programmatically) by stopping
/// sending and receiving data after having dispatched the PAUSE command: the
/// instance receiving is also mandated to stop as soon as possible sending data
/// while it must continue listening for commands (the only one allowed is
/// PAUSE, to exit from the pause mode). This feature is also exploited by the
/// receiving side of the protocol itself to signal the peer when it is
/// necessary to ask the user before being able to continue with the transfer.
///
class SyfftProtocolCommon : public QObject
{
    Q_OBJECT

public:
    /// \brief A special value indicating that the UUID is still unknown.
    static const QString UNKNOWN_UUID;

    ///
    /// \brief The Status enum describes the possible status the SYFFT protocol
    /// can be in.
    ///
    enum class Status {
        New,               ///< \brief Connection not yet attempted.
        Connecting,        ///< \brief Connection in progress.
        Connected,         ///< \brief Connection established.
        InTransfer,        ///< \brief File transfer in progress.
        TransferCompleted, ///< \brief File transfer completed.
        Closing,           ///< \brief Connection closure in progress.
        Closed,            ///< \brief Connection closed.
        Aborted,           ///< \brief Connection aborted.
        PausedByUser,      ///< \brief Connection paused by the local user.
        PausedByPeer       ///< \brief Connection paused by the peer user.
    };
    Q_ENUM(Status)

    ///
    /// \brief Deleted constructor since the class is not instantiable.
    ///
    explicit SyfftProtocolCommon() = delete;

    ///
    /// \brief Destroys the current instance.
    ///
    virtual ~SyfftProtocolCommon();

    ///
    /// \brief Returns the UUID associated to the local user.
    ///
    QString localUuid() const { return m_localUuid; }

    ///
    /// \brief Returns the UUID associated to the peer user.
    ///
    QString peerUuid() const
    {
        QMutexLocker lk(&m_mutex);
        return m_peerUuid;
    }

    ///
    /// \brief Returns the status the protocol is currently in.
    ///
    Status status() const
    {
        QMutexLocker lk(&m_mutex);
        return m_status;
    }

    ///
    /// \brief Returns the statistics about the file transfer.
    ///
    TransferInfo transferInfo();

    ///
    /// \brief Enters or exits the pause mode, depending on the parameter.
    /// \param enterPauseMode a value specifying whether the user wants to enter
    ///        or exit the pause mode.
    ///
    void changePauseMode(bool enterPauseMode);

    ///
    /// \brief Terminates the active connection (if any).
    ///
    void terminateConnection();

signals:
    ///
    /// \brief Signal emitted when the connection status changes.
    /// \param status the status the connection changed to.
    ///
    void statusChanged(Status status);

    ///
    /// \brief Signal emitted when the connection is established.
    ///
    void connected();

    ///
    /// \brief Signal emitted when an accepted transfer is completed.
    ///
    void transferCompleted();

    ///
    /// \brief Signal emitted when the connection is closed.
    ///
    void closed();

    ///
    /// \brief Signal emitted when the connection is aborted.
    ///
    void aborted();

protected slots:
    ///
    /// \brief Closes the active connection (if any).
    ///
    virtual void closeConnection();

    ///
    /// \brief Aborts the active connection (if any).
    ///
    virtual void abortConnection();

    ///
    /// \brief Enters or exits the pause mode, depending on the current status
    /// \param userRequested true if requested by the user or false if requested
    /// by the peer.
    ///
    void togglePauseMode(bool userRequested);

    ///
    /// \brief Function executed when some data is ready to be read from
    /// the socket.
    ///
    virtual void readData() = 0;

    ///
    /// \brief Prints an error message and aborts the connection.
    /// \param message the error message to be printed.
    ///
    void manageError(const QString &message);

protected:
    ///
    /// \brief The data type used to store and transmit a Command.
    ///
    typedef quint8 CommandType;

    ///
    /// \brief The Command enum describes the possible commands used by the
    /// SYFFT Protocol.
    ///
    enum Command {
        ABORT = 0x00, ///< \brief Aborts the connection.
        CLOSE = 0x01, ///< \brief Closes the connection.

        HELLO = 0x02, ///< \brief Starts the connection phase.
        ACK = 0x03,   ///< \brief Completes the connection phase.

        SHARE = 0x10, ///< \brief Starts the transfer session.
        ITEM = 0x11,  ///< \brief Announces a new item of the file list.
        START = 0x12, ///< \brief Starts a file transfer.
        SKIP = 0x13,  ///< \brief Skips a file transfer.
        CHUNK = 0x14, ///< \brief Announces a new chunk of data.

        ACCEPT = 0x20, ///< \brief Accepts a transfer (session or file).
        REJECT = 0x21, ///< \brief Rejects a transfer (session or file).
        COMMIT = 0x22, ///< \brief Commits a file transfer.
        ROLLBK = 0x23, ///< \brief Rollbacks a file transfer.
        STOP = 0x24,   ///< \brief Requests the peer to stop a file transfer.

        PAUSE = 0x30, ///< \brief Enters or exits pause mode.
    };

    /// \brief The maximum length allowed for messages.
    static const int MAX_MSG_LEN = 500;

    /// \brief The maximum buffer size allowed for the socket.
    static const quint64 MAX_BUFFER_SIZE;

    ///
    /// \brief Initializes the common fields of the SYFFT Protocol instances.
    /// \param localUuid the UUID representing the local user.
    /// \param peerUuid the UUID representing the peer user.
    /// \param socket the socket to be used for the communication.
    /// \param parent the parent of the current object.
    ///
    SyfftProtocolCommon(const QString &localUuid, const QString &peerUuid,
                        QTcpSocket *socket, QObject *parent = Q_NULLPTR);

    ///
    /// \brief Increments the file counter and checks if the transfer finished.
    /// \return true if there are still files to be transferred and false
    /// otherwise.
    ///
    bool moveToNextFile();

    ///
    /// \brief Returns a string which identifies the current instance of the
    /// protocol (to be printed to the log).
    ///
    QString logSyfftId() const
    {
        return QString("SyfftProtocol [id = %1]:").arg(m_id);
    }

    ///
    /// \brief Sets the UUID associated to the peer user in a thread-safe
    /// manner.
    /// \param uuid the UUID to be set.
    ///
    void setPeerUuid(const QString &uuid)
    {
        QMutexLocker lk(&m_mutex);
        m_peerUuid = uuid;
    }

    ///
    /// \brief Sets the status the protocol is currently in, in a thread-safe
    /// manner.
    /// \param status the status to be set.
    ///
    void setStatus(Status status)
    {
        QMutexLocker lk(&m_mutex);
        m_status = status;
    }

protected:
    const quint32 m_id; ///< \brief Numerical id of the current instance.

    const QString m_localUuid; ///< \brief The UUID representing the local user.
    /// \brief The UUID representing the peer user (mutex required).
    QString m_peerUuid;

    Status m_status; ///< \brief The current protocol status (mutex required).

    /// \brief The statistics about the file transfer (mutex required).
    QScopedPointer<TransferInfo> m_transferInfo;

    QDir m_basePath; ///< \brief The path the files are relative to.
    /// \brief The list of files to be transferred or received.
    QVector<FileInfo> m_files;

    /// \brief The index identifying the file currently in transfer.
    quint32 m_currentFile;
    /// \brief An object representing the file currently in transfer.
    QScopedPointer<FileInTransfer> m_fileInTransfer;

    /// \brief The socket used for the communication.
    QPointer<QTcpSocket> m_socket;
    /// \brief The data stream associated to the communication channel.
    QScopedPointer<QDataStream> m_stream;

    /// \brief The timer used to measure the total elapsed time (mutex
    /// required).
    QScopedPointer<QElapsedTimer> m_elapsedTimer;
    /// \brief The timer used to measure the time spent in transfer (mutex
    /// required).
    QScopedPointer<QElapsedTimer> m_transferTimer;
    /// \brief The timer used to measure the time spent in pause (mutex
    /// required).
    QScopedPointer<QElapsedTimer> m_pauseTimer;

    /// \brief Indicates whether the user is prevented from toggling the pause
    /// mode.
    bool m_preventUserTogglePause;

    /// \brief The mutex used to protect the members accessed through public
    /// members.
    mutable QMutex m_mutex;

private:
    /// \brief The stack used to store the status when in pause mode.
    QStack<Status> m_oldStatusStack;

    /// \brief The counter used to generate the instance id.
    static QAtomicInteger<quint32> m_counter;
};

#endif // SYFFTPROTOCOLCOMMON_HPP
