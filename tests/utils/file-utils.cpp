/*
 * Copyright 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Charles Kerr <charles.kerr@canonical.com>
 *     Xavi Garcia <xavi.garcia.mena@gmail.com>
 */

#include "tests/utils/file-utils.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QString>
#include <QTemporaryFile>

FileUtils::Info
FileUtils::createDummyFile(const QDir& dir, qint64 filesize)
{
    FileUtils::Info info;
    info.info = QFileInfo();
    info.checksum = QByteArray();

    // NB we want to exercise long filenames, but this cutoff length is arbitrary
    static constexpr int MAX_BASENAME_LEN {200};
    int filename_len = qrand() % MAX_BASENAME_LEN;
    QString basename;
    for (int i=0; i<filename_len; ++i)
        basename += ('a' + char(qrand() % ('z'-'a')));
    basename += QStringLiteral("-XXXXXX");
    auto template_name = dir.absoluteFilePath(basename);

    // fill the file with noise
    QTemporaryFile f(template_name);
    f.setAutoRemove(false);
    if(!f.open())
    {
        qWarning() << "Error opening temporary file: " << f.errorString();
        return info;
    }
    static constexpr qint64 max_step = 1024;
    char buf[max_step];
    qint64 left = filesize;
    while(left > 0)
    {
        int this_step = std::min(max_step, left);
        for(int i=0; i<this_step; ++i)
            buf[i] = 'a' + char(qrand() % ('z'-'a'));
        if (f.write(buf, this_step) < this_step)
        {
            qWarning() << "Error writing to temporary file: " << f.errorString();
        }
        left -= this_step;
    }
    f.close();

    // get a checksum
    if(!f.open())
    {
        qWarning() << "Error opening temporary file: " << f.errorString();
        return info;
    }
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&f);
    const auto checksum = hash.result();
    f.close();

    info.info = QFileInfo(f.fileName());
    info.checksum = checksum;
    return info;
}

bool recursiveFillDirectory(QString const & dirPath, int max_filesize, int & j, QVector<FileUtils::Info> & files, qint64 & filesize_sum, int & max_dirs)
{
    // get the number of files or directories that we will create at this level
    // it will always be less than the number of files remaining to create
    auto nb_items_to_create = qrand() % (files.size() - j);
    if ((files.size() - j) <= 1 )
    {
        nb_items_to_create = 1;
    }

    for (auto i = 0; i < nb_items_to_create && j < files.size(); ++i)
    {
        // decide if it's a file or a directory
        // we create a directory 25% of the time
        if (max_dirs && qrand() % 100 < 25)
        {
            QDir dir(dirPath);
            auto newDirName = QString("Directory_%1").arg(j);
            if (!dir.mkdir(newDirName))
            {
                qWarning() << "Error creating temporary directory " << newDirName << " under " << dirPath;
                return false;
            }

            QDir newDir(QString("%1%2%3").arg(dir.absolutePath()).arg(QDir::separator()).arg(newDirName));

            max_dirs--;

            // fill it
            recursiveFillDirectory(newDir.absolutePath(), max_filesize, j, files, filesize_sum, max_dirs);
        }
        else
        {
            // get the j file and increment the j index
            auto& file = files[j++];
            const auto filesize = qrand() % max_filesize;
            file = FileUtils::createDummyFile(dirPath, filesize);
            if (file.info == QFileInfo())
            {
                return false;
            }
            filesize_sum += file.info.size();
        }
    }
    return true;
}

bool FileUtils::fillTemporaryDirectory(QString const & dir, int max_files_per_test, int max_filesize, int max_dirs)
{
    const auto n_files = std::max(1, (qrand() % max_files_per_test));
    QVector<FileUtils::Info> files (n_files);
    qint64 filesize_sum = 0;
    auto dirs_to_create = max_dirs;
    int j = 0;
    while (j<files.size())
    {
        recursiveFillDirectory(dir, max_filesize, j, files, filesize_sum, dirs_to_create);
    }
    return true;
}

namespace
{

QByteArray
fileChecksum(QString const & fileName, QCryptographicHash::Algorithm hashAlgorithm)
{
    QFile f(fileName);
    if (f.open(QFile::ReadOnly)) {
        QCryptographicHash hash(hashAlgorithm);
        if (hash.addData(&f)) {
            return hash.result();
        }
    }
    return QByteArray();
}

} // anon namespace

QStringList
FileUtils::getFilesRecursively(QString const & dirPath)
{
    QStringList ret;
    QDirIterator iter(dirPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    while(iter.hasNext())
    {
        QFileInfo info(iter.next());
        if (info.isFile())
        {
            ret << info.absoluteFilePath();
        }
        else if (info.isDir())
        {
            ret << getFilesRecursively(info.absoluteFilePath());
        }
    }

    return ret;
}

bool
FileUtils::compareFiles(QString const & filePath1, QString const & filePath2)
{
    QFileInfo info1(filePath1);
    QFileInfo info2(filePath2);
    if (!info1.isFile())
    {
        qWarning() << "Origin file: " << info1.absoluteFilePath() << " does not exist";
        return false;
    }
    if (!info2.isFile())
    {
        qWarning() << "File to compare: " << info2.absoluteFilePath() << " does not exist";
        return false;
    }
    auto checksum1 = fileChecksum(filePath1, QCryptographicHash::Md5);
    auto checksum2 = fileChecksum(filePath1, QCryptographicHash::Md5);
    if (checksum1 != checksum2)
    {
        qWarning() << "Checksum for file: " << filePath1 << " differ";
    }
    return checksum1 == checksum2;
}

bool
FileUtils::compareDirectories(QString const & dir1Path, QString const & dir2Path)
{
    // we only check for files, not directories
    QDir dir1(dir1Path);
    if (!QDir::isAbsolutePath(dir1Path))
    {
        qWarning() << "Error comparing directories: path for directories must be absolute";
        return false;
    }

    if (!checkPathIsDir(dir1Path))
    {
        return false;
    }
    if (!checkPathIsDir(dir2Path))
    {
        return false;
    }

    QStringList filesDir1 = getFilesRecursively(dir1Path);
    QStringList filesDir2 = getFilesRecursively(dir2Path);

    if ( filesDir1.size() != filesDir2.size())
    {
        qWarning() << "Number of files in directories mismatch, dir \""
                   << dir1Path
                   <<  "\" has ["
                   << filesDir1.size()
                   << "], dir \""
                   << dir2Path
                   << "\" has [" << filesDir2.size() << "]";
        return false;
    }
    for (auto file: filesDir1)
    {
        auto filePath2 = file;
        filePath2.remove(0, dir1Path.length());
        filePath2 = dir2Path + filePath2;
        if (!compareFiles(file, filePath2))
        {
            qWarning() << "File [" << file << "] and file [" << filePath2 << "] are not equal";
            return false;
        }
    }
    return true;
}

bool FileUtils::checkPathIsDir(QString const &dirPath)
{
    QFileInfo dirInfo = QFileInfo(dirPath);
    if (!dirInfo.isDir())
    {
        if (!dirInfo.exists())
        {
            qWarning() << "Directory: [" << dirPath << "] does not exist";
            return false;
        }
        else
        {
            qWarning() << "Path: [" << dirPath << "] is not a directory";
            return false;
        }
    }
    return true;
}
