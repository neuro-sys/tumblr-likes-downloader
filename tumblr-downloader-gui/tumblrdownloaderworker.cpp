/**
 *  Tumblr Downloader is an application that authenticates to your Tumblr
 *  account, and downloads all the liked photos to your computer.
 *  Copyright (C) 2017  Firat Salgur

 *  This file is part of Tumblr Downloader

 *  Tumblr Downloader is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  Tumblr Downloader is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Tumblr Downloader.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tumblrdownloaderworker.h"

#include <QProcess>
#include <iostream>
#include <QCoreApplication>
#include <QFile>
#include <QThread>
#include <QTemporaryDir>

TumblrDownloaderWorker::TumblrDownloaderWorker(QObject *parent) : QObject(parent)
{
    process = new QProcess(this);
    running = false;
}

TumblrDownloaderWorker::~TumblrDownloaderWorker()
{
    delete process;
}

void TumblrDownloaderWorker::run()
{
    running = true;
#ifdef Q_OS_WIN
    QFile file(":/bin/tumblr-downloader.exe");
#else
    QFile file(":/bin/tumblr-downloader");
#endif
    QFileInfo fileInfo(file.fileName());
    QTemporaryDir dir;

    QString targetFilePath = dir.path() + QDir::separator() + fileInfo.fileName();
    QFile targetFile(targetFilePath);

    if (!file.copy(targetFilePath)) {
        emit emitStatus("Something went wrong at file: " + QString(__FILE__) + " at func: " + QString(__FUNCTION__));
    }

    targetFile.setPermissions(QFile::ExeGroup | QFile::ExeOther | QFile::ExeOther | QFile::ExeUser);

    emit emitStatus("Starting...");
    process->start(targetFilePath);
    if (!process->waitForReadyRead()) {
        emit emitStatus("The process did not start successfully.");
    }

    char buf[1024];
    buf[0] = 0;
    do {
        if (QThread::currentThread()->isInterruptionRequested() || !running) {
            break;
        }
        process->readLine(buf, sizeof(buf));
        if (strlen(buf) > 0) {
            emit emitStatus(QString::fromLocal8Bit(buf));
        }
        buf[0] = 0;
    } while (!process->waitForFinished(1) && running);

    process->terminate();
    process->kill();
    emit emitFinished();
}
