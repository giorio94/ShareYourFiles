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

#include "transferlist.hpp"

#include <Logger.h>

#include <QDir>
#include <QFileInfo>

///
/// The instance is generated starting from the list of absolute paths referring
/// to files or directories that are going to be scheduled for transfer: it is
/// mandatory that the paths given as parameters are absolute and all referring
/// to elements in the same directory. This strong constraint is due to the
/// necessity to convert the paths obtained through the SYFP Protocol (that
/// follows the given format) to relative paths with respect to a base directory
/// that can be sent through the SYFFT Protocol.
///
/// The list building proceeds by adding creating a FileInfo instance for each
/// valid file specified; in case of directories, on the other hand, the process
/// continues recursively until all files have been included.
///
TransferList::TransferList(const QStringList &pathsList) : m_totalBytes(0)
{
    // No files specified: just return
    if (pathsList.isEmpty()) {
        return;
    }

    // Build the list of file and directory names
    QStringList items;
    foreach (const QString &path, pathsList) {
        QFileInfo info(path);

        // Check if the path is absolute and the file name is not empty
        if (!info.isAbsolute() || info.fileName().isEmpty()) {
            LOG_WARNING() << "TransferList: skipped invalid path" << path;
            continue;
        }

        // If the base path is still null, set it
        if (m_basePath.isNull()) {
            m_basePath = QDir::cleanPath(info.absolutePath());
        }

        // Otherwise check if the path of the current file is the same of
        // the saved one
        if (m_basePath != QDir::cleanPath(info.absolutePath())) {
            LOG_ERROR() << "TransferList: detected files or directories "
                           "with different base paths: it is not possible "
                           "to continue generating the list.";
            return;
        }

        // Add the name to the list
        items << info.fileName();
    }

    // Build the actual list
    buildFileList(items);
}

///
/// The function builds the list of the files to be transferred, according
/// to the items specified as parameter. In particular, it scans the directory
/// specified as base path and, for each file or directory found, checks if the
/// items list contains it: in that case the addToFileList() function is
/// executed to recursively add all files to the list. The items must be names
/// of files or directories directly children of the base directory, and they
/// must not contain trailing slashes.
///
void TransferList::buildFileList(QStringList items)
{
    // Get the list of files and directories contained in the base path and
    // check which are in the items list.
    foreach (const QFileInfo &child,
             QDir(m_basePath)
                 .entryInfoList(QDir::AllEntries | QDir::Readable |
                                    QDir::NoDotAndDotDot,
                                QDir::Name | QDir::DirsLast)) {

        // Get the name of the file or the directory
        QString itemName = child.fileName();
        // Check if it is in the list of items, and in that case add it
        // to the file list
        if (items.contains(itemName)) {
            items.removeAll(itemName);
            addToFileList(child);
        }
    }

    // Check the remaining values which are invalid
    foreach (const QString &item, items) {
        LOG_WARNING() << "TransferList: skipped invalid file or directory"
                      << item;
    }
}

///
/// The function, given a QFileInfo object, checks if it is valid and readable;
/// in that case, if it represents a file, a new FileInfo instance is created
/// and
/// added to the list while, if it represents a directory, the process is
/// repeated recursively with all elements inside it. Symbolic links are skipped
/// to avoid the complexities given by the possibility of loops.
///
void TransferList::addToFileList(const QFileInfo &item)
{
    QString relativePath =
        QDir(m_basePath).relativeFilePath(item.absoluteFilePath());

    // Check if the file/directory exists and is readable
    if (!item.exists() || !item.isReadable()) {
        LOG_ERROR() << "TransferList: file or directory does not exist"
                       " or is not readable"
                    << item.absoluteFilePath();
        return;
    }

    // If it is a symbolic link, skip it
    if (item.isSymLink()) {
        LOG_WARNING() << "TransferList: symbolic link detected but not (yet)"
                         " supported"
                      << item.absoluteFilePath();
        return;
    }

    // If the object represents a file, add it to the list
    if (item.isFile()) {
        quint64 size = static_cast<quint64>(item.size());
        FileInfo fileInfo(relativePath, size, item.lastModified());

        // Check if the built instance is valid
        if (!fileInfo.valid()) {
            LOG_ERROR() << "TransferList: skipped invalid file"
                        << item.absoluteFilePath();
            return;
        }

        m_files.push_back(fileInfo);
        m_totalBytes += size;

        return;
    }

    // If the object represents a directory, add all the elements to the list
    if (item.isDir()) {
        QDir dir(item.absoluteFilePath());
        foreach (const QFileInfo &child,
                 dir.entryInfoList(QDir::AllEntries | QDir::Readable |
                                       QDir::NoDotAndDotDot,
                                   QDir::Name | QDir::DirsLast)) {
            // Process in recursive manner the entry
            addToFileList(child);
        }
        return;
    }

    // If the object belongs to none of the previous types, print an error
    LOG_ERROR() << "TransferInfo: file or directory type not detected or not"
                   " supported"
                << item.absoluteFilePath();
}
