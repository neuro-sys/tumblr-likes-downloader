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
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "tumblrdownloaderworker.h"

#include <QColor>
#include <QThread>
#include <iostream>
#include <QSettings>
#include <QCloseEvent>
#include <QProcessEnvironment>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    loadSettings();
    initThread();

    QFont statusFont("Courier New");
    statusFont.setStyleHint(QFont::Monospace);
    ui->statusTextArea->setFont(statusFont);
    ui->statusTextArea->setTextColor(QColor(0, 64, 0, 255));

    ui->targetLocationLineEdit->installEventFilter(this);
    if (ui->targetLocationLineEdit->text().isEmpty()) {
        ui->targetLocationLineEdit->setText(QCoreApplication::applicationDirPath());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initThread()
{
    thread = new QThread(this);
    tumblrDownloaderWorker = new TumblrDownloaderWorker();
    tumblrDownloaderWorker->moveToThread(thread);

    connect(thread, SIGNAL(started()), tumblrDownloaderWorker, SLOT(run()));
    connect(tumblrDownloaderWorker, SIGNAL(emitStatus(QString)), this, SLOT(receiveStatus(QString)));
    connect(tumblrDownloaderWorker, SIGNAL(emitFinished()), this, SLOT(receiveFinished()) );
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (thread->isRunning()) {
        ui->pushButton->setText("Cancelling...");
        ui->statusTextArea->append("* Cancelling...");
        tumblrDownloaderWorker->running = false;
        thread->quit();
        thread->wait(100);
    }

    saveSettings();
    event->accept();
}


void MainWindow::loadSettings()
{
    settings = new QSettings ("Tumblr Downloader", "Tumblr Downloader");

    ui->blogNameLineEdit->setText(settings->value("BLOG_IDENTIFIER").toString());
    ui->consumerKeyLineEdit->setText(settings->value("CONSUMER_KEY").toString());
    ui->consumerSecretLineEdit->setText(settings->value("CONSUMER_SECRET").toString());
    //ui->defaultCallbackURLLineEdit->setText(settings->value("CALLBACK_URL").toString());
    ui->targetLocationLineEdit->setText(settings->value("TARGET_LOCATION").toString());
    ui->startOffsetLineEdit->setText(settings->value("START_OFFSET").toString());
    ui->debugModeCheckBox->setChecked(settings->value("DEBUG_MODE").toBool());
    ui->downloadToSameDirectoryCheckBox->setChecked(settings->value("DOWNLOAD_NO_DIRS").toBool());

    if (ui->startOffsetLineEdit->text().length() == 0) {
        ui->startOffsetLineEdit->setText("0");
    }
}

void MainWindow::saveSettings()
{
    settings->setValue("BLOG_IDENTIFIER", ui->blogNameLineEdit->text());
    settings->setValue("CONSUMER_KEY", ui->consumerKeyLineEdit->text());
    settings->setValue("CONSUMER_SECRET", ui->consumerSecretLineEdit->text());
    //settings->setValue("CALLBACK_URL", "http://localhost:8080/tumblr/callback");
    settings->setValue("TARGET_LOCATION", ui->targetLocationLineEdit->text());
    settings->setValue("START_OFFSET", ui->startOffsetLineEdit->text());
    settings->setValue("DEBUG_MODE", ui->debugModeCheckBox->isChecked());
    settings->setValue("DOWNLOAD_NO_DIRS", ui->downloadToSameDirectoryCheckBox->isChecked());
    settings->sync();

    qputenv("BLOG_IDENTIFIER", ui->blogNameLineEdit->text().toStdString().c_str());
    qputenv("CONSUMER_KEY", ui->consumerKeyLineEdit->text().toStdString().c_str());
    qputenv("CONSUMER_SECRET", ui->consumerSecretLineEdit->text().toStdString().c_str());
    qputenv("CALLBACK_URL", "http://localhost:8080/tumblr/callback");
    qputenv("TARGET_LOCATION", ui->targetLocationLineEdit->text().toStdString().c_str());
    qputenv("START_OFFSET", ui->startOffsetLineEdit->text().toStdString().c_str());
    qputenv("DEBUG_MODE", ui->debugModeCheckBox->isChecked() ? "true" : "false");
    qputenv("DOWNLOAD_NO_DIRS", ui->downloadToSameDirectoryCheckBox->isChecked() ? "true" : "false");
}

void MainWindow::on_pushButton_clicked()
{
    if (tumblrDownloaderWorker->running) {
        ui->pushButton->setText("Cancelling...");
        ui->statusTextArea->append("* Cancelling...");
        tumblrDownloaderWorker->running = false;
        ui->pushButton->setText("Download");
    } else {
        saveSettings();
        initThread();
        thread->start();
        tumblrDownloaderWorker->running = true;
        ui->pushButton->setText("Cancel");
        ui->statusTextArea->clear();
        ui->statusTextArea->append("* Starting...\n");
    }
}

void MainWindow::receiveStatus(const QString &status)
{
    ui->statusTextArea->insertPlainText(status);
    ui->statusTextArea->ensureCursorVisible();
}

void MainWindow::receiveFinished()
{
    ui->statusTextArea->append("* Finished...");
    ui->pushButton->setText("Download");
    tumblrDownloaderWorker->running = false;
}

bool MainWindow::eventFilter(QObject* object, QEvent* event)
{
    if(object == ui->targetLocationLineEdit && event->type() == QEvent::MouseButtonRelease) {
        QString path = QFileDialog::getExistingDirectory (this, "Target Destination");
        if (path != NULL && !path.isEmpty()) {
            ui->targetLocationLineEdit->setText(path);
            return true;
        }
    }

    return false;
}

