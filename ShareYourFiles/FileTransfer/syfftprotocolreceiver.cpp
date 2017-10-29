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

#include "syfftprotocolreceiver.hpp"
#include "Common/common.hpp"
#include "fileintransfer.hpp"
#include "transferinfo.hpp"

#include <Logger.h>

#include <QDataStream>
#include <QElapsedTimer>
#include <QTcpSocket>
#include <QTimer>
#include <QUuid>


// Register QSharedPointer<SyfftProtocolSharingRequest> to the qt meta type
// system
static MetaTypeRegistration<QSharedPointer<SyfftProtocolSharingRequest>>
    sharingReqRegisterer("QSharedPointer<SyfftProtocolSharingRequest>");
// Register QSharedPointer<SyfftProtocolDuplicatedFile> to the qt meta type
// system
static MetaTypeRegistration<QSharedPointer<SyfftProtocolDuplicatedFile>>
    duplicatedFileRegisterer("QSharedPointer<SyfftProtocolDuplicatedFile>");
// Register SyfftProtocolReceiver::DuplicatedFileAction to the qt meta type
// system
static MetaTypeRegistration<SyfftProtocolReceiver::DuplicatedFileAction>
    actionRegisterer("DuplicatedFileAction");

///
/// The instance is initialized by executing the SyfftProtocolCommon constructor
/// for what concerns the common parts and the timeout is started. The signals
/// emitted from the socket are blocked until the acceptConnection() method is
/// executed: this is done to allow the user to connect all the necessary slots
/// to the signals of this class before starting the connection.
///
/// \see SyfftProtocolReceiver::acceptConnection()
///
SyfftProtocolReceiver::SyfftProtocolReceiver(const QString &localUuid,
                                             QTcpSocket *socket,
                                             QObject *parent)
        : SyfftProtocolCommon(localUuid, UNKNOWN_UUID, socket, parent),
          m_defaultDFAction(DuplicatedFileAction::Ask)
{
    // Block the signals until acceptConnection is executed
    m_socket->blockSignals(true);
}

///
/// The function moves the instance (that must be in New status) to the
/// Connecting status and unblocks the signals from the socket, therefore
/// allowing the connection to proceed.
///
/// It also stores the pair object/slot that will be in charge of accepting or
/// rejecting the sharing request when received from the peer; the method must
/// be a slot with the signature
/// void (QSharedPointer<SyfftProtocolSharingRequest>),
/// belonging to the shareHandler object: in case the invocation will fail,
/// the sharing request will be automatically rejected.
///
/// The pair object/slot delegated to handle received files with the same name
/// of ones already on disk is also stored; the method must be a slot with the
/// signature void (QSharedPointer<SyfftProtocolDuplicatedFile>), belonging to
/// the duplicateHandler object: in case the invocation will fail, the involved
/// file transfer will be automatically rejected.
///
void SyfftProtocolReceiver::acceptConnection(QObject *shareHandler,
                                             const char *shareHandlerSlot,
                                             QObject *duplicateHandler,
                                             const char *duplicateHandlerSlot)
{
    QByteArray shareHandlerSlotBA(shareHandlerSlot);
    QByteArray duplicateHandlerSlotBA(duplicateHandlerSlot);
    QTimer::singleShot(0, this, [this, shareHandler, shareHandlerSlotBA,
                                 duplicateHandler, duplicateHandlerSlotBA]() {

        // The function must be executed only once
        LOG_ASSERT_X(
            m_status == Status::New,
            qUtf8Printable(logSyfftId() + " instance not in New status"));

        // Set the handlers
        m_shareHandler = shareHandler;
        m_shareHandlerSlot = shareHandlerSlotBA;
        m_duplicatedHandler = duplicateHandler;
        m_duplicatedHandlerSlot = duplicateHandlerSlotBA;

        // Unblock the signals
        m_socket->blockSignals(false);

        // Start the timer to measure the total elapsed time
        QMutexLocker lk(&m_mutex);
        m_elapsedTimer->start();
        lk.unlock();

        setStatus(Status::Connecting);
        emit statusChanged(Status::Connecting);

        // Read buffered data
        readData();
    });
}

///
/// A new transaction is immediately started in order to prevent short reads
/// (the transaction is committed only if all the data composing a command
/// has been correctly read) and the command code is read; depending on
/// the received code, the correct operation is then performed. The process
/// continues until there is data available.
///
void SyfftProtocolReceiver::readData()
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

        case Command::ACK:
            if (ackCommand())
                break;
            return;

        case Command::SHARE:
            if (shareCommand())
                break;
            return;

        case Command::ITEM:
            if (itemCommand())
                break;
            return;

        case Command::START:
            if (startCommand())
                break;
            return;

        case Command::SKIP:
            if (skipCommand())
                break;
            return;

        case Command::CHUNK:
            if (chunkCommand())
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
/// It is immediately checked if the HELLO command is expected (i.e. the
/// connection is in Connecting status with the peer UUID still unknown) and
/// in negative case the connection is aborted. The method, then, tries to
/// read the expected data following the HELLO command (the peer UUID, as
/// a 16 bytes array in Rfc4122 format) and, in case it is read correctly,
/// the response is sent to the peer (in the form of an HELLO command,
/// followed by the local UUID, as a 16 bytes array in Rfc4122 format).
///
bool SyfftProtocolReceiver::helloCommand()
{
    // Check if the hello command is expected
    if (m_status != Status::Connecting ||
        peerUuid() != SyfftProtocolReceiver::UNKNOWN_UUID) {
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

    // Save the peer UUID
    setPeerUuid(QUuid::fromRfc4122(peerUuid).toString());

    // Answer to the peer
    *m_stream << static_cast<CommandType>(Command::HELLO);
    QByteArray uuid = QUuid(m_localUuid).toRfc4122();
    if (m_stream->writeRawData(uuid.constData(), uuid.length()) !=
        uuid.length()) {
        manageError("Short write");
        return false;
    }

    return true;
}

///
/// It is immediately checked if the ACK command is expected (i.e. the
/// connection is in Connecting status with the peer UUID already discovered)
/// and in negative case the connection is aborted. The protocol, then, moves
/// to the Connected status and the statusChanged() and connected() signals
/// are emitted.
///
bool SyfftProtocolReceiver::ackCommand()
{
    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // Check if the hello command is expected
    if (m_status != Status::Connecting ||
        peerUuid() == SyfftProtocolReceiver::UNKNOWN_UUID) {
        manageError("Unexpected ACK command received");
        return false;
    }

    // Move to the connected status
    setStatus(Status::Connected);
    LOG_INFO() << qUtf8Printable(logSyfftId()) << "connected to"
               << qUtf8Printable(m_peerUuid);

    emit statusChanged(Status::Connected);
    emit connected();

    return true;
}

///
/// It is immediately checked if the SHARE command is expected (i.e. the
/// connection is in Connecting status) and in negative case the connection
/// is aborted.
///
/// In case the SHARE command has not already been received, the function tries
/// to read the data following it: the total number of files (a 32 bits unsigned
/// number), the total size of the files (a 64 bits unsigned number) and the
/// textual message attached to the request (a 32 bits unsigned number
/// representing the length followed by the characters in UTF8 format).
///
/// On the other hand, if it represents the end of the request (i.e. all the
/// information about the files has already been correctly received) the
/// connection is paused, a new SyfftProtocolSharingRequest is built and the
/// slot in charge of accepting or rejecting the connection is executed.
///
bool SyfftProtocolReceiver::shareCommand()
{
    // SHARE request start
    if (m_status == Status::Connected && m_transferInfo->m_totalFiles == 0 &&
        m_shareMsg.isNull()) {

        // Try reading the number of files and the total size
        quint32 totalFiles;
        quint64 totalBytes;
        *m_stream >> totalFiles;
        *m_stream >> totalBytes;

        // Try reading the sender message
        QByteArray message;
        *m_stream >> message;

        // Still missing data
        if (!m_stream->commitTransaction()) {
            return false;
        }

        // Check if Qt containers can handle the number of elements to be
        // received (indexes are int and not unsigned)
        if (totalFiles >= INT_MAX) {
            manageError("Too many files requested for transfer");
            return false;
        }

        // Update the transfer information
        QMutexLocker lk(&m_mutex);
        m_transferInfo->m_totalFiles = totalFiles;
        m_transferInfo->m_totalBytes = totalBytes;
        lk.unlock();

        m_shareMsg = QString::fromUtf8(message).left(MAX_MSG_LEN);
        if (m_shareMsg.isNull()) {
            // Convert the message from null to empty
            m_shareMsg = QString("");
        }
        return true;
    }

    // SHARE request end
    if (m_status == Status::Connected &&
        static_cast<quint32>(m_files.count()) == m_transferInfo->m_totalFiles &&
        !m_shareMsg.isNull()) {

        // Still missing data
        if (!m_stream->commitTransaction()) {
            return false;
        }

        quint32 totalFiles = m_transferInfo->m_totalFiles;
        quint64 totalBytes = 0;

        // Check if the advertised number of bytes corresponds to the actual sum
        foreach (const FileInfo &info, m_files) {
            totalBytes += info.size();
        }

        if (totalBytes != m_transferInfo->m_totalBytes) {
            m_files.clear();
            manageError(
                "Invalid FileInfo received following the sharing request");
            return false;
        }

        // Print the file information to the LOG
        LOG_INFO() << qUtf8Printable(logSyfftId())
                   << "sharing request received for" << totalFiles << "files -"
                   << qUtf8Printable(sizeToHRFormat(totalBytes));

        // Build a new SyfftProtocolSharingRequest instance
        SyfftProtocolSharingRequest *request = new SyfftProtocolSharingRequest(
            m_peerUuid, totalFiles, totalBytes, m_files, m_shareMsg);

        // Connect the handlers
        connect(request, &SyfftProtocolSharingRequest::accepted, this,
                &SyfftProtocolReceiver::acceptSharingRequest);
        connect(request, &SyfftProtocolSharingRequest::rejected, this,
                &SyfftProtocolReceiver::rejectSharingRequest);
        connect(this, &SyfftProtocolCommon::aborted, request,
                &SyfftProtocolSharingRequest::connectionAborted);

        // Enter pause mode and prevent the user from changing it
        togglePauseMode(true);
        m_preventUserTogglePause = true;

        QSharedPointer<SyfftProtocolSharingRequest> ptr(request,
                                                        &QObject::deleteLater);
        // Invoke the method in charge of accepting or rejecting the share
        // request
        bool result = QMetaObject::invokeMethod(
            m_shareHandler, m_shareHandlerSlot.constData(),
            Q_ARG(QSharedPointer<SyfftProtocolSharingRequest>, ptr));
        // Failed invoking the method
        if (!result) {
            LOG_ERROR() << qUtf8Printable(logSyfftId()) << "failed invoking"
                        << QString(m_shareHandlerSlot);

            // When the SyfftProtocolSharingRequest instance goes out of scope,
            // the sharing request is automatically rejected by the destructor
        }
        return true;
    }

    // The command is not expected
    m_stream->commitTransaction();
    manageError("Unexpected SHARE command received");
    return false;
}

///
/// It is immediately checked if the ITEM command is expected (i.e. the
/// connection is in Connecting status and not too many file information has
/// already been received) and in negative case the connection is aborted.
/// The function, then, proceeds by trying to read the information about a file
/// scheduled for transfer and, in case they are correct, adding them to the
/// files list.
///
bool SyfftProtocolReceiver::itemCommand()
{
    // Check if the ITEM command is expected
    if (m_status != Status::Connected ||
        static_cast<quint32>(m_files.count()) >= m_transferInfo->m_totalFiles) {

        m_stream->commitTransaction();
        manageError("Unexpected ITEM command received");
        return false;
    }

    // Try reading the file information
    FileInfo file;
    *m_stream >> file;

    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // Wrong file info data received
    if (!file.valid()) {
        m_stream->rollbackTransaction();
        manageError("Invalid FileInfo received following the sharing request");
        return false;
    }

    m_files.push_back(file);
    return true;
}

///
/// It is immediately checked if the START command is expected (i.e. the
/// connection is in InTransfer status and the m_fileInTransfer variable is
/// empty) and in negative case the connection is aborted. The method then
/// proceeds by creating a new FileInTransferWriter instance to represent
/// the file to be received: in case an error occurs, the file is
/// automatically rejected. If the file does not already exists, the ACCEPT
/// command is sent to the peer, otherwise either the performDFAction() method
/// is delegated to perform the default action or the decision is left to the
/// user (the connection is paused, a new SyfftProtocolDuplicatedFile is built
/// and the slot in charge of managing the situation is executed).
///
bool SyfftProtocolReceiver::startCommand()
{
    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // Check if the START command is expected
    if (m_status != Status::InTransfer || m_fileInTransfer) {
        manageError("Unexpected START command received");
        return false;
    }

    // Otherwise create a new fileInTransferWriter instance
    m_fileInTransfer.reset(new FileInTransferWriter(
        m_basePath, m_files.at(static_cast<int>(m_currentFile))));

    // If an error occurred opening the file, reject the transfer
    if (m_fileInTransfer->error()) {
        rejectFileTransfer();
        return true;
    }

    // If the file does not already exist, accept the transfer
    if (!m_fileInTransfer->exists()) {
        acceptFileTransfer();
        return true;
    }

    LOG_INFO() << qUtf8Printable(logSyfftId())
               << "detected a file with the same name"
               << m_fileInTransfer->relativePath();

    // Duplicated file detected and default action already chosen
    if (m_defaultDFAction != DuplicatedFileAction::Ask) {
        performDFAction(m_defaultDFAction);
        return true;
    }

    // Otherwise prepare the request to the user
    SyfftProtocolDuplicatedFile *request = new SyfftProtocolDuplicatedFile(
        m_peerUuid, QFileInfo(m_fileInTransfer->absolutePath()),
        m_files.at(static_cast<int>(m_currentFile)));

    // Connect the handler
    connect(request, &SyfftProtocolDuplicatedFile::chosen, this,
            [this](DuplicatedFileAction action, bool all) {

                // Exit pause mode
                m_preventUserTogglePause = false;
                togglePauseMode(true);

                // The connection has already been aborted
                if (m_status == Status::Aborted) {
                    return;
                }

                // Change the default if necessary
                if (all) {
                    m_defaultDFAction = action;
                }

                // Perform the actual action
                performDFAction(action);
            });
    connect(this, &SyfftProtocolCommon::aborted, request,
            &SyfftProtocolDuplicatedFile::connectionAborted);

    // Enter pause mode and prevent the user from changing it
    togglePauseMode(true);
    m_preventUserTogglePause = true;

    QSharedPointer<SyfftProtocolDuplicatedFile> ptr(request,
                                                    &QObject::deleteLater);
    // Invoke the method in charge of deciding the action
    bool result = QMetaObject::invokeMethod(
        m_duplicatedHandler, m_duplicatedHandlerSlot.constData(),
        Q_ARG(QSharedPointer<SyfftProtocolDuplicatedFile>, ptr));
    // Failed invoking the method
    if (!result) {
        LOG_ERROR() << qUtf8Printable(logSyfftId()) << "failed invoking"
                    << QString(m_duplicatedHandlerSlot);

        // When the SyfftProtocolDuplicatedFile instance goes out of scope,
        // the default action (keep the current file) is automatically
        // chosen
        // by the destructor
    }

    return true;
}

///
/// It is immediately checked if the SKIP command is expected (i.e. the
/// connection is in InTransfer status and the m_fileInTransfer variable is
/// empty) and in negative case the connection is aborted. The method then
/// updates the transfer information, and sends the REJECT command to
/// confirm the reception.
///
bool SyfftProtocolReceiver::skipCommand()
{
    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // Check if the SKIP command is expected
    if (m_status != Status::InTransfer || m_fileInTransfer) {
        manageError("Unexpected SKIP command received");
        return false;
    }

    LOG_INFO() << qUtf8Printable(logSyfftId()) << "file transfer skipped"
               << m_files.at(static_cast<int>(m_currentFile)).name();

    // Send the REJECT command to confirm the reception
    *m_stream << static_cast<CommandType>(Command::REJECT);

    // Update the transfer information
    QMutexLocker lk(&m_mutex);
    m_transferInfo->m_skippedFiles++;
    m_transferInfo->m_skippedBytes +=
        m_files.at(static_cast<int>(m_currentFile)).size();
    lk.unlock();

    m_files[static_cast<int>(m_currentFile)].setStatus(
        FileInfo::Status::TransferFailed);

    moveToNextFile();
    return true;
}

///
/// It is immediately checked if the CHUNK command is expected (i.e. the
/// connection is in InTransfer status and the m_fileInTransfer variable is
/// set) and in negative case the connection is aborted. The method then
/// tries to read the data following the CHUNK command (a 32 bit unsigned
/// integer representing the data length followed by the actual data) and,
/// in case of success, attempts to write the buffer to the file. If the
/// last operation fails, the transfer is rollbacked and, if not yet done,
/// the STOP command is sent.
///
bool SyfftProtocolReceiver::chunkCommand()
{
    // Check if the CHUNK command is expected
    if (m_status != Status::InTransfer || !m_fileInTransfer) {
        m_stream->commitTransaction();
        manageError("Unexpected CHUNK command received");
        return false;
    }

    // Try reading the length of the chunk
    quint32 length;
    *m_stream >> length;

    // Still missing data
    if (m_stream->status() != QDataStream::Status::Ok) {
        m_stream->rollbackTransaction();
        return false;
    }

    // Length greater than allowed
    if (length > FileInTransfer::MAX_CHUNK_SIZE) {
        m_stream->commitTransaction();
        manageError("Oversized file chunk detected");
        return false;
    }

    // Try reading the actual data
    QByteArray buffer;
    buffer.resize(static_cast<int>(length));
    m_stream->readRawData(buffer.data(), static_cast<int>(length));

    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // Try writing the data to the file
    if (m_fileInTransfer->processNextDataChunk(buffer)) {
        // In case of success, add the transferred bytes
        QMutexLocker lk(&m_mutex);
        m_transferInfo->m_transferredBytes += length;
        return true;
    }

    // In case of error, and if not yet done, send the STOP command
    else if (!m_fileInTransfer->rollbacked()) {
        m_fileInTransfer->rollback();
        *m_stream << static_cast<CommandType>(Command::STOP);
    }

    return true;
}

///
/// It is immediately checked if the COMMIT command is expected (i.e. the
/// connection is in InTransfer status and the m_fileInTransfer variable is
/// set) and in negative case the connection is aborted. The method, then,
/// proceeds trying to commit the local copy of the file: in case of success
/// the COMMIT command is sent to the peer, while in case of error the
/// ROLLBK one is sent; the transfer information are then updated and the next
/// file transfer is started.
///
bool SyfftProtocolReceiver::commitCommand()
{
    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // Check if the COMMIT command is expected
    if (m_status != Status::InTransfer || !m_fileInTransfer) {
        manageError("Unexpected COMMIT command received");
        return false;
    }

    // Check if it is possible to commit the file
    if (m_fileInTransfer->commit()) {
        LOG_INFO() << qUtf8Printable(logSyfftId()) << "file transfer committed"
                   << m_fileInTransfer->relativePath();

        // Acknowledge the commit
        *m_stream << static_cast<CommandType>(Command::COMMIT);

        // Update the transfer information
        m_files[static_cast<int>(m_currentFile)].setStatus(
            FileInfo::Status::Transferred);

        QMutexLocker lk(&m_mutex);
        m_transferInfo->m_transferredFiles++;
        lk.unlock();
    }

    // In case of failure, send the ROLLBK command
    else {
        LOG_INFO() << qUtf8Printable(logSyfftId()) << "file transfer rollbacked"
                   << m_fileInTransfer->relativePath();

        // Request the rollback
        m_fileInTransfer->rollback();
        *m_stream << static_cast<CommandType>(Command::ROLLBK);

        // Update the transfer information
        m_files[static_cast<int>(m_currentFile)].setStatus(
            FileInfo::Status::TransferFailed);

        QMutexLocker lk(&m_mutex);
        m_transferInfo->m_skippedFiles++;
        m_transferInfo->m_skippedBytes += m_fileInTransfer->remainingBytes();
        lk.unlock();
    }

    // In any case, move to the next file
    m_fileInTransfer.reset();
    moveToNextFile();

    return true;
}

///
/// It is immediately checked if the ROLLBK command is expected (i.e. the
/// connection is in InTransfer status and the m_fileInTransfer variable is
/// set) and in negative case the connection is aborted. The method, then,
/// proceeds by acknowledging the command, updating the transfer information
/// and starting the next file transfer.
///
bool SyfftProtocolReceiver::rollbkCommand()
{
    // Still missing data
    if (!m_stream->commitTransaction()) {
        return false;
    }

    // Check if the ROLLBK command is expected
    if (m_status != Status::InTransfer || !m_fileInTransfer) {
        manageError("Unexpected ROLLBK command received");
        return false;
    }

    LOG_INFO() << qUtf8Printable(logSyfftId()) << "file transfer rollbacked"
               << m_fileInTransfer->relativePath();

    // Acknowledge the rollback
    m_fileInTransfer->rollback();
    *m_stream << static_cast<CommandType>(Command::ROLLBK);

    // Otherwise update the transfer information and move to the next file
    m_files[static_cast<int>(m_currentFile)].setStatus(
        FileInfo::Status::TransferFailed);

    QMutexLocker lk(&m_mutex);
    m_transferInfo->m_skippedFiles++;
    m_transferInfo->m_skippedBytes += m_fileInTransfer->remainingBytes();
    lk.unlock();

    m_fileInTransfer.reset();
    moveToNextFile();
    return true;
}

///
/// It is immediately checked if the CLOSE command is expected, otherwise
/// the connection is aborted. The connection is then terminated through the
/// closeConnection() method.
///
bool SyfftProtocolReceiver::closeCommand()
{
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

///
/// The function is in charge of sending the ACCEPT command to the peer,
/// followed by the attached textual message; the status is then changed to
/// InTransfer and the statusChanged() signal is emitted.
///
void SyfftProtocolReceiver::acceptSharingRequest(const QString &path,
                                                 const QString &message)
{
    // Exit pause mode
    m_preventUserTogglePause = false;
    togglePauseMode(true);

    // The connection has already been aborted
    if (m_status == Status::Aborted) {
        return;
    }

    // Check if the specified path is feasible
    QDir directory(path);
    LOG_INFO() << qUtf8Printable(logSyfftId()) << "base path:"
               << QDir::home().relativeFilePath(directory.path());
    if (!directory.mkpath(".") || !directory.isReadable()) {
        manageError("Invalid base path specified");
        return;
    }
    m_basePath = path;

    // Send the ACCEPT command and the message to the peer (as UTF8 encoded
    // string)
    QString trimmed = message.left(SyfftProtocolReceiver::MAX_MSG_LEN);
    *m_stream << static_cast<CommandType>(Command::ACCEPT);
    *m_stream << trimmed.toUtf8();

    // Start the timer to measure the transfer time
    QMutexLocker lk(&m_mutex);
    m_transferTimer->start();
    lk.unlock();

    // Move to InTransfer status
    LOG_INFO() << qUtf8Printable(logSyfftId()) << "sharing request accepted";
    setStatus(Status::InTransfer);
    emit statusChanged(Status::InTransfer);

    moveToNextFile();
}


///
/// The function is in charge of sending the REJECT command to the peer,
/// followed by the attached textual message; the connection is then
/// politely closed.
///
void SyfftProtocolReceiver::rejectSharingRequest(const QString &message)
{
    // Exit pause mode
    m_preventUserTogglePause = false;
    togglePauseMode(true);

    // The connection has already been aborted
    if (m_status == Status::Aborted) {
        return;
    }

    // Send the REJECT command and the message to the peer (as UTF8 encoded
    // string)
    QString trimmed = message.left(SyfftProtocolReceiver::MAX_MSG_LEN);
    *m_stream << static_cast<CommandType>(Command::REJECT);
    *m_stream << trimmed.toUtf8();

    // Update the transfer information
    QMutexLocker lk(&m_mutex);
    m_transferInfo->m_skippedFiles = m_transferInfo->m_totalFiles;
    m_transferInfo->m_skippedBytes = m_transferInfo->m_totalBytes;
    lk.unlock();

    // Close the connection
    LOG_INFO() << qUtf8Printable(logSyfftId()) << "sharing request rejected";
    closeConnection();
}

///
/// The function is in charge of updating the transfer information and of
/// sending the ACCEPT command to the peer.
///
void SyfftProtocolReceiver::acceptFileTransfer()
{
    LOG_INFO() << qUtf8Printable(logSyfftId()) << "file transfer accepted"
               << m_fileInTransfer->relativePath();
    *m_stream << static_cast<CommandType>(Command::ACCEPT);

    // Update the transfer information
    m_files[static_cast<int>(m_currentFile)].setStatus(
        FileInfo::Status::InTransfer);
}

///
/// The function is in charge of updating the transfer information and of
/// sending the REJECT command to the peer.
///
void SyfftProtocolReceiver::rejectFileTransfer()
{
    LOG_INFO() << qUtf8Printable(logSyfftId()) << "file transfer rejected"
               << m_fileInTransfer->relativePath();
    *m_stream << static_cast<CommandType>(Command::REJECT);

    // Update the transfer information
    m_fileInTransfer.reset();
    m_files[static_cast<int>(m_currentFile)].setStatus(
        FileInfo::Status::TransferRejected);

    QMutexLocker lk(&m_mutex);
    m_transferInfo->m_skippedFiles++;
    m_transferInfo->m_skippedBytes +=
        m_files.at(static_cast<int>(m_currentFile)).size();
    lk.unlock();

    moveToNextFile();
}

///
/// The action specified by the parameter is executed: in case the file is
/// to be replaced the file transfer is accepted, while if it is to be kept the
/// file transfer is rejected. On the other hand, if both files must be
/// kept, a suffix like _1 is added between the name and the extension of the
/// file: different suffixes are tried until a name that does not correspond to
/// an existing file is found (the transfer is accepted) or an error occurs
/// (the file transfer is rejected).
///
void SyfftProtocolReceiver::performDFAction(DuplicatedFileAction action)
{
    LOG_ASSERT_X(action != DuplicatedFileAction::Ask,
                 qUtf8Printable(logSyfftId() + " action not specified"));

    // Requested to replace the current file
    if (action == DuplicatedFileAction::Replace) {
        acceptFileTransfer();
        return;
    }

    // Requested to keep the current file
    if (action == DuplicatedFileAction::Keep) {
        rejectFileTransfer();
        return;
    }

    // Requested to add a suffix to the received file
    FileInfo currentFile = m_files.at(static_cast<int>(m_currentFile));
    QFileInfo fileInfo(currentFile.name());

    quint8 counter = 1;
    QString path = currentFile.path();
    QString name = fileInfo.baseName();
    QString ext = fileInfo.completeSuffix();

    // Loop until a unique suffix is found
    while (counter != 0) {

        // Recompute the whole relative path
        QString newPath;

        // Empty name (e.g. hidden files in Unix)
        if (name.isEmpty()) {
            newPath = QString("%1/.%2_%3").arg(path).arg(ext).arg(counter);
        }

        // File with no extension
        else if (ext.isEmpty()) {
            newPath = QString("%1/%2_%3").arg(path).arg(name).arg(counter);
        }

        // File with both name and extension
        else {
            newPath = QString("%1/%2_%3.%4")
                          .arg(path)
                          .arg(name)
                          .arg(counter)
                          .arg(ext);
        }

        // Create a new FileInfo and FileInTransferWriter instances
        FileInfo newFile(QDir::cleanPath(newPath), currentFile.size(),
                         currentFile.lastModified());
        QScopedPointer<FileInTransferWriter> newFileWriter(
            new FileInTransferWriter(m_basePath, newFile));

        // If an error occurred, reject the transfer
        if (newFileWriter->error()) {
            break;
        }

        // If the file does not already exist, start the transfer
        if (!newFileWriter->exists()) {
            m_files[static_cast<int>(m_currentFile)] = newFile;
            m_fileInTransfer.reset(newFileWriter.take());
            acceptFileTransfer();
            return;
        }

        // Otherwise, try with the next suffix
        counter++;
    }

    LOG_WARNING() << qUtf8Printable(logSyfftId()) << "failed renaming the file"
                  << m_fileInTransfer->relativePath();
    rejectFileTransfer();
}
