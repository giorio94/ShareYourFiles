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

#include "fileintransfer.hpp"

#include <Logger.h>

///
/// The member fields are initialized, partially according to the parameters,
/// and partially to default values.
///
FileInTransfer::FileInTransfer(const QDir &basePath, const FileInfo &fileInfo)
        : m_fileInfo(fileInfo),
          m_absolutePath(
              QDir::cleanPath(basePath.absoluteFilePath(fileInfo.filePath()))),
          m_error(!fileInfo.valid()),
          m_exists(basePath.exists(fileInfo.filePath())),
          m_remainingBytes(fileInfo.size()),
          m_transferStarted(false),
          m_committed(false),
          m_rollbacked(false)
{
}


/******************************************************************************/


///
/// After having saved the parameters, it is checked if the information stored
/// in the FileInfo instance are still valid: in that case it is attempted to
/// open the file itself. It is possible to check if the operation succeeded
/// through the error() method.
///
FileInTransferReader::FileInTransferReader(const QDir &basePath,
                                           const FileInfo &fileInfo)
        : FileInTransfer(basePath, fileInfo)
{
    // If the FileInfo object is not valid or if the file changed, just return
    if (m_error || updated()) {
        m_error = true;
        return;
    }

    // Try to open the file
    m_file.setFileName(m_absolutePath);
    if (!m_file.open(QFile::ReadOnly)) {
        m_error = true;
        LOG_ERROR() << "FileInTransferReader: failed opening" << m_absolutePath
                    << "-" << m_file.errorString();
    }
}

///
/// The function tries to read a chunk of data from the opened file. In case
/// the read succeeds, the data is saved in the buffer; a boolean value is
/// returned to indicate the outcome of the operation.
///
bool FileInTransferReader::processNextDataChunk(QByteArray &buffer)
{
    m_transferStarted = true;

    // An error occurred or already read completely
    if (m_error || m_remainingBytes == 0 || !m_file.isOpen()) {
        m_error = true;
        return false;
    }

    // Compute the amount of bytes to be read
    quint64 toRead =
        m_remainingBytes > MAX_CHUNK_SIZE ? MAX_CHUNK_SIZE : m_remainingBytes;
    int toReadInt = static_cast<int>(toRead);

    // Read the actual data
    buffer.resize(toReadInt);
    if (m_file.read(buffer.data(), toReadInt) != toReadInt) {
        // In case of short read, set the error
        QString errString =
            (m_file.atEnd()) ? "End of file reached" : m_file.errorString();
        LOG_ERROR() << "FileInTransferWriter: short read" << m_absolutePath
                    << "-" << errString;

        m_error = true;
        return false;
    }

    // Success
    m_remainingBytes -= toRead;
    return true;
}

///
/// The function verifies if it is possible to commit the file transfer, that is
/// if the file has been completely read without errors and its information has
/// not changed since the beginning of the transfer. The file is then closed and
/// the outcome is returned.
///
bool FileInTransferReader::commit()
{
    // Set transfer started to avoid problems in case of empty files
    m_transferStarted = true;

    // If already committed, just return
    if (m_committed)
        return m_committed;

    // Check if something went wrong and rollback the transfer
    if (m_error || m_remainingBytes != 0 || !m_file.atEnd() || updated()) {
        rollback();
        return false;
    }

    // Otherwise commit it
    m_file.close();
    m_committed = true;
    return true;
}

///
/// The function sets the error status and closes the file. A boolean value is
/// returned indicating whether the operation succeeded or not.
///
bool FileInTransferReader::rollback()
{
    // If already committed
    if (m_committed)
        return false;

    // Otherwise close file and set the error status
    m_file.close();
    m_rollbacked = true;
    m_error = true;
    return true;
}

///
/// The function checks if the cached information stored in the FileInfo object
/// are still correct, that is if the file exists  and is readable, and if the
/// size and the last modified date have not been changed (false is returned),
/// or if something changed (true is returned).
///
bool FileInTransferReader::updated()
{
    // Build the absolute path to the file and the QFileInfo instance
    QFileInfo info(m_absolutePath);

    // Check if something changed
    return !(m_exists && info.exists() && info.isFile() && info.isReadable() &&
             static_cast<quint64>(info.size()) == m_fileInfo.size() &&
             info.lastModified() == m_fileInfo.lastModified());
}


/******************************************************************************/


///
/// After having saved the parameters, it is checked if the specified file
/// already exists and then it is attempted to open the file. It is possible to
/// check if the operation succeeded through the error() method.
///
FileInTransferWriter::FileInTransferWriter(const QDir &basePath,
                                           const FileInfo &fileInfo)
        : FileInTransfer(basePath, fileInfo)
{
    // If the FileInfo object is not valid, just return
    if (m_error)
        return;

    // Create the path where the file will be stored (if necessary)
    if (!basePath.mkpath(m_fileInfo.path())) {
        m_error = true;
        LOG_ERROR() << "FileInTransferWriter: failed creating directory"
                    << m_absolutePath;
    }

    // Try to open the file
    m_file.setFileName(m_absolutePath);
    if (!m_file.open(QFile::WriteOnly)) {
        m_error = true;
        LOG_ERROR() << "FileInTransferWriter: failed opening" << m_absolutePath
                    << "-" << m_file.errorString();
    }
}

///
/// The function tries to write a chunk of data to the opened file. The boolean
/// value returned indicates whether the operation succeeded or failed.
///
bool FileInTransferWriter::processNextDataChunk(QByteArray &buffer)
{
    m_transferStarted = true;

    // An error occurred, or already written completely
    if (m_error || !m_file.isOpen() ||
        m_remainingBytes < static_cast<quint64>(buffer.length())) {
        m_error = true;
        return false;
    }

    // Write the actual data
    if (m_file.write(buffer.data(), buffer.length()) != buffer.length()) {
        // In case of short write, set the error
        LOG_ERROR() << "FileInTransferWriter: short write" << m_absolutePath
                    << "-" << m_file.errorString();

        m_error = true;
        return false;
    }

    // Success
    m_remainingBytes -= static_cast<quint64>(buffer.length());
    return true;
}

///
/// The function verifies if it is possible to commit the file transfer, that
/// is if the file has been completely written without errors. The file is
/// then committed to disk and closed. The outcome of the operation is returned.
///
bool FileInTransferWriter::commit()
{
    // Set transfer started to avoid problems in case of empty files
    m_transferStarted = true;

    // If already committed, just return
    if (m_committed)
        return m_committed;

    // If an error occurred, the file is not open or not the whole file has
    // been transferred, it is impossible to commit
    if (!m_file.isOpen() || m_error || m_remainingBytes != 0) {
        rollback();
        return false;
    }

    // Otherwise check if it is possible to commit the file
    if (m_file.commit()) {
        m_committed = true;
        return true;
    }

    // Rollback it
    rollback();
    return false;
}

///
/// The function discards the data saved to the file and sets the error status.
///
bool FileInTransferWriter::rollback()
{
    // If already committed
    if (m_committed)
        return false;

    // Otherwise cancel writing the file and set the error status
    m_file.cancelWriting();
    m_rollbacked = true;
    m_error = true;
    return true;
}
