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

#include "fileinfo.hpp"

#include <Logger.h>

#include <QDataStream>
#include <QDir>
#include <QFileInfo>

///
/// The FileInfo instance is created by storing the parameters to the member
/// fields. It is then checked if the file path can be considered valid, and in
/// that case the file name and the path are stored; otherwise, an invalid
/// instance is generated.
///
FileInfo::FileInfo(const QString &filePath, quint64 size,
                   const QDateTime &lastModified)
        : m_valid(false),
          m_filePath(filePath),
          m_size(size),
          m_lastModified(lastModified),
          m_status(Status::Scheduled)
{

    // If the file name looks strange, just return
    QFileInfo file(m_filePath);
    if (m_filePath != QDir::cleanPath(m_filePath) ||
        m_filePath.startsWith("../") || !file.isRelative()) {
        return;
    }

    // Compute name and path
    m_name = file.fileName();
    m_path = file.path();

    m_valid = true;
}

///
/// The FileInfo instance is written to the stream according to the following
/// format: the file name (a 32 bits unsigned number representing the file name
/// length followed by the actual characters in UTF8 format), the file size
/// (a 64 bits unsigned number), and the date/time of last modification of the
/// file (according to the QDateTime serialization format). The instance to
/// be written must be valid and scheduled for transfer.
///
/// \see operator>>()
///
QDataStream &operator<<(QDataStream &stream, const FileInfo &fileInfo)
{
    if (!fileInfo.m_valid || fileInfo.m_status != FileInfo::Status::Scheduled) {
        LOG_ERROR() << "FileInfo: trying to output an invalid instance.";
        return stream;
    }

    // Output all fields to the stream
    stream << fileInfo.m_filePath.toUtf8();
    stream << fileInfo.m_size;
    stream << fileInfo.m_lastModified;

    return stream;
}

///
/// The FileInfo instance is written to the stream according to the following
/// format: the file name (a 32 bits unsigned number representing the file name
/// length followed by the actual characters in UTF8 format), the file size
/// (a 64 bits unsigned number), and the date/time of last modification of the
/// file (according to the QDateTime serialization format).
///
/// In case of invalid data read from the stream, that is its final status is
/// different from Ok, an invalid FileInfo instance is created. The same happens
/// if the file name looks suspicious (e.g it contains . or .., or ends with a
/// slash)
///
/// \see operator<<()
///
QDataStream &operator>>(QDataStream &stream, FileInfo &fileInfo)
{
    fileInfo.m_valid = false;

    // Read the file name (converted from UTF8)
    QByteArray fileName;
    stream >> fileName;
    fileInfo.m_filePath = QString::fromUtf8(fileName);

    // Read the remaining fields
    stream >> fileInfo.m_size;
    stream >> fileInfo.m_lastModified;

    // If the stream status is not ok, something went wrong
    if (stream.status() != QDataStream::Status::Ok) {
        return stream;
    }

    // If the file name looks strange, just return
    QFileInfo file(fileInfo.m_filePath);
    if (fileInfo.m_filePath != QDir::cleanPath(fileInfo.m_filePath) ||
        fileInfo.m_filePath.startsWith("../") || !file.isRelative()) {
        return stream;
    }

    // Compute name and path
    fileInfo.m_name = file.fileName();
    fileInfo.m_path = file.path();

    // Otherwise, mark the instance as valid
    fileInfo.m_valid = true;
    return stream;
}
