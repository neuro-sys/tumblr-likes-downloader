#include "tumblrdownloaderworker.h"

#include <QProcess>
#include <iostream>
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>

TumblrDownloaderWorker::TumblrDownloaderWorker(QObject *parent) : QObject(parent)
{
    process = new QProcess(this);
}

TumblrDownloaderWorker::~TumblrDownloaderWorker()
{
    delete process;
}

void TumblrDownloaderWorker::run()
{
    this->running = true;
#ifdef Q_OS_WIN
    QFile file(":/bin/tumblr-downloader.exe");
#else
    QFile file(":/bin/tumblr-downloader");
#endif
    QFileInfo fileInfo(file.fileName());
    QTemporaryDir dir;

    QString targetFilePath = dir.path() + QDir::separator() + fileInfo.fileName();

    if (!file.copy(targetFilePath)) {
        emit emitImageURL("Something went wrong at file: " + QString(__FILE__) + " at func: " + QString(__FUNCTION__));
    }

    QFile targetFile(targetFilePath);
    targetFile.setPermissions(QFile::ExeGroup | QFile::ExeOther | QFile::ExeOther | QFile::ExeUser);

    process->start(targetFilePath);
    process->waitForReadyRead();

    char buf[1024];
    buf[0] = 0;
    do {
        process->readLine(buf, sizeof(buf));
        if (strlen(buf) > 0) {
            emit emitImageURL(QString::fromLocal8Bit(buf).simplified());
        }
        buf[0] = 0;
    } while (!process->waitForFinished(100) && running);

    process->kill();
    emit receiveTumblrImageURLFinished();
}
