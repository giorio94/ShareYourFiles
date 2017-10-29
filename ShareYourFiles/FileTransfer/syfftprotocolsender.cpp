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

#include "syfftprotocolsender.hpp"
#include "Common/common.hpp"
#include "fileintransfer.hpp"
#include "transferinfo.hpp"
#include "transferlist.hpp"

#include <Logger.h>

#include <QDataStream>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTimer>
#include <QUuid>

// Register SyfftProtocolSender::PeerStatus to the qt meta type system
static MetaTypeRegistration<SyfftProtocolSender::PeerStatus>
    peerStatusRegisterer("PeerStatus");


///
/// The instance is initialized by executing the SyfftProtocolCommon
/// constructor for what concerns the common parts and by setting the
/// members according to the parameters. The handler to start the connection
/// handshake when the socket gets connected is attached, and the same
/// is done for the one in charge of sending file chunks when the socket
/// write buffer empties.
///
SyfftProtocolSender::SyfftProtocolSender(const QString &localUuid,
                                         const QString &peerUuid,
                                         quint32 address, quint16 port,
                                         PeerStatus peerMode, QObject *parent)
        : SyfftProtocolCommon(localUuid, peerUuid, new QTcpSocket(), parent),
          m_peerStatus(peerMode),
          m_peerAddress(address),
          m_peerPort(port)
{
    // Connect the handler executed when the control connection is established
    connect(m_socket, &QAbstractSocket::connected, this, [this]() {

        // Send the HELLO command, followed by the local UUID
        *m_stream << static_cast<CommandType>(Command::HELLO);
        QByteArray uuid = QUuid(m_localUuid).toRfc4122();
        if (m_stream->writeRawData(uuid.constData(), uuid.length()) !=
            uuid.length()) {
            manageError("Short write");
        }
    });

    // Connect the handler executed when some bytes are written to the socket
    connect(m_socket, &QTcpSocket::bytesWritten, this, [this]() {

        // If a file is in transfer, send the next chunks
        if (m_status == Status::InTransfer && m_fileInTransfer &&
            m_fileInTransfer->transferStarted() &&
            !m_fileInTransfer->transferCompleted()) {
            sendDataChunks();
        }
    });
}

///
/// The function, that must be executed only when the instance is in New status,
/// is in charge of storing the list of files to be sent to the peer and of
/// starting a new connection to the peer. In case this phase completes
/// correctly, the transfer request will be eventually sent, along with the
/// specified message.
///
void SyfftProtocolSender::sendFiles(const TransferList &files,
                                    const QString &message)
{
    // Use a timer to execute the operations from the thread owning this object
    QTimer::singleShot(0, this, [this, files, message]() {

        LOG_ASSERT_X(
            m_status == Status::New,
            qUtf8Printable(logSyfftId() + " instance not in New status"));

        // Set the base directory and the files to be shared
        m_basePath = QDir(files.basePath());
        m_files = files.m_files;

        // Set the total number and size of the files
        QMutexLocker lk(&m_mutex);
        m_transferInfo->m_totalFiles = files.totalFiles();
        m_transferInfo->m_totalBytes = files.totalBytes();
        lk.unlock();

        LOG_INFO() << qUtf8Printable(logSyfftId()) << "base path:"
                   << QDir::home().relativeFilePath(m_basePath.path());
        LOG_INFO() << qUtf8Printable(logSyfftId()) << files.totalFiles()
                   << "files scheduled for transfer -"
                   << qUtf8Printable(sizeToHRFormat(files.totalBytes()));

        // Save the message with the size limited to MAX_MSG_LEN characters
        m_shareMsg = message.left(SyfftProtocolSender::MAX_MSG_LEN);

        connectToPeer();
    });
}

///
/// In case the status is equal to the previous one, nothing is done; otherwise,
/// the new status is set, the connection is aborted and the peerStatusChanged()
/// signal is emitted.
///
void SyfftProtocolSender::updatePeerStatus(
    SyfftProtocolSender::PeerStatus peerStatus)
{
    // Use a timer to execute the operations from the thread owning this object
    QTimer::singleShot(0, this, [this, peerStatus]() {

        // If the status is the same of the previous just return
        if (m_peerStatus == peerStatus) {
            return;
        }

        // Update the cached status
        setPeerStatus(peerStatus);
        LOG_INFO() << qUtf8Printable(logSyfftId()) << "status changed to"
                   << qUtf8Printable(enum2str(peerStatus));

        // Abort the current connection (if any)
        abortConnection();

        emit peerStatusChanged(peerStatus);
    });
}

///
/// In case the address is equal to the previous one, nothing is done;
/// otherwise, the new address is set and the connection is aborted.
///
void SyfftProtocolSender::updatePeerAddress(quint32 address, quint16 port)
{
    // Use a timer to execute the operations from the thread owning this object
    QTimer::singleShot(0, this, [this, address, port]() {

        // If the address is the same just return
        if (m_peerAddress == address && m_peerPort == port) {
            return;
        }

        // Otherwise update the cached address
        m_peerAddress = address;
        m_peerPort = port;
        LOG_INFO() << qUtf8Printable(logSyfftId()) << "address updated"
                   << qUtf8Printable(QHostAddress(address).toString()) << "@"
                   << port;

        // Abort the current connection (if any)
        abortConnection();
    });
}

///
/// The function attempts the connection to the peer; the connection status
/// is changed to Connecting and the statusChanged() signal is emitted.
///
void SyfftProtocolSender::connectToPeer()
{
    // Connect the control socket to the peer
    LOG_INFO() << qUtf8Printable(logSyfftId()) << "connecting to"
               << qUtf8Printable(m_peerUuid) << "-"
               << qUtf8Printable(QHostAddress(m_peerAddress).toString()) << "@"
               << m_peerPort;

    m_socket->connectToHost(QHostAddress(m_peerAddress), m_peerPort);

    // Start the timer to measure the total elapsed time
    QMutexLocker lk(&m_mutex);
    m_elapsedTimer->start();
    lk.unlock();

    setStatus(Status::Connecting);
    emit statusChanged(Status::Connecting);
}

///
/// A new transaction is immediately started in order to prevent short reads
/// (the transaction is committed only if all the data composing a command
/// has been correctly read) and then the command code is read; depending on
/// the received code, the correct operation is then performed. The process
/// continues until there is data available.
///
void SyfftProtocolSender::readData()
{
    // In case the user paused the instance, do not read anything
    if (m_status == Status::PausedByUser)
        return;

    // Continue until data is still available
    while (m_socket->bytesAvailable() >=
           static_cast<qint64>(sizeof(CommandType))) {

        // Start a new transaction
        m_stream->startTransaction();

        // Read the received command
        CommandType command;
        *m_stream >> command;

        // Check if the command has been received correctly
        if (m_stream->status() != QDataStream::Status::Ok) {
            m_stream->rollbackTransaction();
            return;
        }

        // Perform an action depending on the command code
        switch (command) {

        case Command::HELLO:
            if (helloCommand())
                break;
            return;

        case Command::ACCEPT:
            if (acceptCommand())
                break;
            return;

        case Command::REJECT:
            if (rejectCommand())
                break;
            return;

        case Command::COMMIT:
            if (commitCommand())
                break;
            return;

        case Command::ROLLBK:
            if (rollbkCommand())
                break;
            return;

        case Command::STOP:
            if (stopCommand())
                break;
            return;

        case Command::CLOSE:
            if (closeCommand())
                break;
            return;

        case Command::PAUSE:
            m_stream->commitTransaction();
            togglePauseMode(false);
            break;

        case Command::ABORT:
            m_stream->commitTransaction();
            manageError("ABORT requested by the peer");
            return;

        default:
            m_stream->commitTransaction();
            manageError("Unrecognized command received");
            return;
        }
    }
}

///
/// The function initially verifies if all the scheduled files have already
/// been transferred: in that case, the status is changed to TransferCompleted,
/// the status changed signal is emitted and finally the connection is closed.
/// Otherwise, a new FileInTransferReader instance is created to represent
/// the file to be sent: according to the result, a Start or a SKIP command
/// is sent to the peer.
///
void SyfftProtocolSender::transferNextFile()
{
    // Advance to the next file
    m_fileInTransfer.reset();
    if (!moveToNextFile()) {
        return;
    }

    // Create a new FileInTransferReader instance
    m_fileInTransfer.reset(new FileInTransferReader(
        m_basePath, m_files.at(static_cast<int>(m_currentFile))));

    // If not created correctly, send the SKIP command
    if (m_fileInTransfer->error()) {
        *m_stream << static_cast<CommandType>(Command::SKIP);
        LOG_ERROR() << qUtf8Printable(logSyfftId()) << "file transfer skipped"
                    << m_fileInTransfer->relativePath();
    }
    // Otherwise send the START command
    else {
        *m_stream << static_cast<CommandType>(Command::START);
        LOG_INFO() << qUtf8Printable(logSyfftId()) << "file transfer started"
                   << m_fileInTransfer->relativePath();
    }
}

///
/// The function, in case the socket write buffer is not full, proceeds by
/// trying to read the next chunk of data from the file: in case of success,
/// the buffered data is sent to the peer following the CHUNK command while,
/// in case of error, the ROLLBK command is sent and the transfer is
/// interrupted.
/// The process continues until either the buffer is full or the file end is
/// reached: in the latter case, it is verified if all the file has been
/// transferred correctly and the COMMIT or the ROLLBACK command is sent
/// accordingly.
///
void SyfftProtocolSender::sendDataChunks()
{
    // Continue writing until the maximum buffer size has been reached
    while (static_cast<quint64>(m_socket->bytesToWrite()) <
           SyfftProtocolCommon::MAX_BUFFER_SIZE) {

        // No more data to be transferred
        if (m_fileInTransfer->remainingBytes() == 0) {

            // If everything ok, commit the file, otherwise rollback it
            CommandType command =
                m_fileInTransfer->commit() ? Command::COMMIT : Command::ROLLBK;

            *m_stream << command;
            return;
        }

        // Read the next chunk of data
        QByteArray buffer;

        // If some error occurred, rollback the transfer
        if (!m_fileInTransfer->processNextDataChunk(buffer)) {
            m_fileInTransfer->rollback();
            *m_stream << static_cast<CommandType>(Command::ROLLBK);
            return;
        }

        // Send the data and update the transfer info
        *m_stream << static_cast<CommandType>(Command::CHUNK);
        *m_stream << buffer;

        QMutexLocker lk(&m_mutex);
        m_transferInfo->m_transferredBytes +=
            static_cast<quint64>(buffer.length());
        lk.unlock();
    }
}

///
/// It is immediately checked if the HELLO command is expected (i.e. the
/// connection is in Connecting status) and in negative case the connection
/// is aborted. The method, then, tries to read the expected data following
/// the HELLO command (the peer UUID, as a 16 bytes array in Rfc4122 format).
/// In case it is read correctly and corresponds to the expected UUID locally
/// cached, the ACK command is sent, the status is changed to Connected
/// and the statusChanged() and the connected() signals are emitted. The SHARE
/// command, followed by the number of files, their total size and the optional
/// message to the peer are then immediately sent, followed by the complete list
/// of files to be shared (each one preceded by the ITEM command). The SHARE
/// command is finally sent again to complete the list.
///
bool SyfftProtocolSender::helloCommand()
{
    // Check if the HELLO command is expected
    if (m_status != Status::Connecting) {
        m_stream->commitTransaction();
        manageError("Unexpected HELLO command received");
        return false;
    }

    // Try reading the UUID
    QByteArray peerUuid;
    peerUuid.resize(Constants::UUID_LEN);
    m_stream->readRawData(peerUuid.data(), Constants::UUID_LEN);

    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // Check if the UUID is the expected one
    if (m_peerUuid != QUuid::fromRfc4122(peerUuid).toString()) {
        manageError("Unexpected peer UUID received");
        return false;
    }

    // Send the ACK command
    *m_stream << static_cast<CommandType>(Command::ACK);

    // Move to the connected status
    setStatus(Status::Connected);
    LOG_INFO() << qUtf8Printable(logSyfftId()) << "connected to"
               << qUtf8Printable(m_peerUuid);

    // Emit the signals
    emit statusChanged(Status::Connected);
    emit connected();

    // Advertise the list of files to be transferred
    // Share command, number of files and total size
    *m_stream << static_cast<CommandType>(Command::SHARE);

    QMutexLocker lk(&m_mutex);
    *m_stream << m_transferInfo->totalFiles();
    *m_stream << m_transferInfo->totalBytes();
    lk.unlock();

    // Message to the peer (as UTF8 encoded string)
    *m_stream << m_shareMsg.toUtf8();
    LOG_INFO() << qUtf8Printable(logSyfftId()) << "sharing request sent";

    // Information about each file
    foreach (const FileInfo file, m_files) {
        *m_stream << static_cast<CommandType>(Command::ITEM);
        *m_stream << file;
    }

    // Send again the share command to end the list
    *m_stream << static_cast<CommandType>(Command::SHARE);
    return true;
}

///
/// It is immediately checked if the ACCEPT command is expected, otherwise the
/// connection is aborted.
///
/// In case the command follows the sharing request, the textual message
/// attached is read, the status is changed to InTransfer, the statusChanged()
/// and accepted() signals are emitted and finally the actual files transfer
/// starts.
///
/// In case the command follows a file transfer request, that is if the file is
/// ready to be shared, the first data chunks are sent.
///
bool SyfftProtocolSender::acceptCommand()
{
    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // If the SHARE command has been sent and not yet acknowledged
    if (m_status == Status::Connected) {
        // Read the message
        QByteArray message;
        *m_stream >> message;

        LOG_INFO() << qUtf8Printable(logSyfftId())
                   << "sharing request accepted";

        // Start the timer to measure the transfer time
        QMutexLocker lk(&m_mutex);
        m_transferTimer->start();
        lk.unlock();

        setStatus(Status::InTransfer);

        // Emit the signals
        QString string = QString::fromUtf8(message).left(MAX_MSG_LEN);
        emit statusChanged(Status::InTransfer);
        emit accepted(string);

        // Start transferring the first file
        transferNextFile();
        return true;
    }

    // If the START command has been sent and not yet acknowledged
    if (m_status == Status::InTransfer && m_fileInTransfer &&
        !m_fileInTransfer->error() && !m_fileInTransfer->transferStarted()) {

        LOG_INFO() << qUtf8Printable(logSyfftId()) << "file transfer accepted"
                   << m_fileInTransfer->relativePath();

        // Start sending the actual data
        m_files[static_cast<int>(m_currentFile)].setStatus(
            FileInfo::Status::InTransfer);

        sendDataChunks();
        return true;
    }

    // Command not expected
    manageError("Unexpected ACCEPT command received");
    return false;
}

///
/// It is immediately checked if the REJECT command is expected, otherwise the
/// connection is aborted.
///
/// In case the command follows the sharing request, the textual message
/// attached is read, the rejected() signal is emitted and the connection is
/// closed.
///
/// In case the command follows a file transfer request, that is if the file is
/// ready to be shared, the transfer information are updated adding the skipped
/// file and, then, the transfer moves to the next file.
///
bool SyfftProtocolSender::rejectCommand()
{
    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // If the SHARE command has been sent and not yet acknowledged
    if (m_status == Status::Connected) {
        // Read the message
        QByteArray message;
        *m_stream >> message;

        LOG_INFO() << qUtf8Printable(logSyfftId())
                   << "sharing request rejected";

        // Update the transfer information
        QMutexLocker lk(&m_mutex);
        m_transferInfo->m_skippedFiles = m_transferInfo->m_totalFiles;
        m_transferInfo->m_skippedBytes = m_transferInfo->m_totalBytes;
        lk.unlock();

        setStatus(Status::TransferCompleted);
        // Emit the signals
        QString string = QString::fromUtf8(message).left(MAX_MSG_LEN);
        emit statusChanged(Status::TransferCompleted);
        emit rejected(string);

        closeConnection();
        return true;
    }

    // If the START command has been sent and not yet acknowledged
    if (m_status == Status::InTransfer && m_fileInTransfer &&
        !m_fileInTransfer->transferStarted()) {

        LOG_INFO() << qUtf8Printable(logSyfftId()) << "file transfer rejected"
                   << m_fileInTransfer->relativePath();

        // Update the transfer information and move to the next file
        QMutexLocker lk(&m_mutex);
        m_transferInfo->m_skippedFiles++;
        m_transferInfo->m_skippedBytes += m_fileInTransfer->remainingBytes();
        lk.unlock();

        m_files[static_cast<int>(m_currentFile)].setStatus(
            m_fileInTransfer->error() ? FileInfo::Status::TransferFailed
                                      : FileInfo::Status::TransferRejected);

        transferNextFile();
        return true;
    }

    // Command not expected
    manageError("Unexpected REJECT command received");
    return false;
}

///
/// It is immediately checked if the COMMIT command is expected (i.e. the
/// connection is in InTransfer status and the file has been committed)
/// and in negative case the connection is aborted. The method, then, proceeds
/// by updating the transfer information and starting the next file transfer.
///
bool SyfftProtocolSender::commitCommand()
{
    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // Check if the COMMIT command is expected
    if (m_status != Status::InTransfer || !m_fileInTransfer ||
        !m_fileInTransfer->committed()) {

        manageError("Unexpected COMMIT command received");
        return false;
    }

    LOG_INFO() << qUtf8Printable(logSyfftId()) << "file transfer committed"
               << m_fileInTransfer->relativePath();

    // Otherwise update the transfer information and move to the next file
    m_files[static_cast<int>(m_currentFile)].setStatus(
        FileInfo::Status::Transferred);

    QMutexLocker lk(&m_mutex);
    m_transferInfo->m_transferredFiles++;
    lk.unlock();

    transferNextFile();
    return true;
}

///
/// It is immediately checked if the ROLLBK command is expected (i.e. the
/// connection is in InTransfer status and the file transfer is either ready
/// to commit or to rollback) and in negative case the connection is aborted.
/// The method, then, proceeds updating the transfer information and starting
/// the next file transfer.
///
bool SyfftProtocolSender::rollbkCommand()
{
    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // Check if the ROLLBK command is expected
    if (m_status != Status::InTransfer || !m_fileInTransfer ||
        !m_fileInTransfer->transferCompleted()) {
        manageError("Unexpected ROLLBK command received");
        return false;
    }

    m_fileInTransfer->rollback();
    LOG_INFO() << qUtf8Printable(logSyfftId()) << "file transfer rollbacked"
               << m_fileInTransfer->relativePath();

    // Otherwise update the transfer information and move to the next file
    m_files[static_cast<int>(m_currentFile)].setStatus(
        FileInfo::Status::TransferFailed);

    QMutexLocker lk(&m_mutex);
    m_transferInfo->m_skippedFiles++;
    m_transferInfo->m_skippedBytes += m_fileInTransfer->remainingBytes();
    lk.unlock();

    transferNextFile();
    return true;
}

///
/// It is immediately checked if the STOP command is expected (i.e. the
/// connection is in InTransfer status and the file transfer has already
/// started) and in negative case the connection is aborted. The method, then,
/// proceeds, in case the transfer of the current file is still running, by
/// stopping it and sending the ROLLBK command to the peer.
///
bool SyfftProtocolSender::stopCommand()
{
    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // Check if the STOP command is expected
    if (m_status != Status::InTransfer || !m_fileInTransfer ||
        !m_fileInTransfer->transferStarted()) {

        manageError("Unexpected STOP command received");
        return false;
    }

    // In case the current file has not yet been transferred completely,
    // rollback it and send the ROLLBK command
    if (!m_fileInTransfer->transferCompleted()) {
        m_fileInTransfer->rollback();
        *m_stream << static_cast<CommandType>(Command::ROLLBK);
    }
    return true;
}

///
/// It is immediately checked if the CLOSE command is expected, otherwise the
/// connection is aborted. The connection is then terminated through the
/// closeConnection() method.
///
bool SyfftProtocolSender::closeCommand()
{
    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // Connection already closed
    if (m_status == Status::Closed) {
        return false;
    }

    // Check if the CLOSE command is expected
    if (m_status != Status::TransferCompleted && m_status != Status::Closing) {
        manageError("Unexpected CLOSE command received");
        return false;
    }

    closeConnection();
    return true;
}
