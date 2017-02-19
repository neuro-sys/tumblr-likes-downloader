#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "tumblrdownloaderworker.h"

#include <QThread>
#include <iostream>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    thread = new QThread(this);
    tumblrDownloaderWorker = new TumblrDownloaderWorker();

    tumblrDownloaderWorker->moveToThread(thread);

    connect(thread, SIGNAL(started()), tumblrDownloaderWorker, SLOT(run()));
    connect(tumblrDownloaderWorker, SIGNAL(emitImageURL(QString)), this, SLOT(receiveTumblrImageURL(QString)));
    connect(tumblrDownloaderWorker, SIGNAL(receiveTumblrImageURLFinished()), thread, SLOT(quit()) );
    connect(tumblrDownloaderWorker, SIGNAL(finished()), thread, SLOT(quit()) );

    loadSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::loadSettings()
{
    settings = new QSettings ("tumblr-downloader.config", QSettings::IniFormat);

    ui->userNameLineEdit->setText(settings->value("USERNAME").toString());
    ui->passwordLineEdit->setText(settings->value("PASSWORD").toString());
    ui->blogNameLineEdit->setText(settings->value("BLOG_IDENTIFIER").toString());
    ui->consumerKeyLineEdit->setText(settings->value("CONSUMER_KEY").toString());
    ui->consumerSecretLineEdit->setText(settings->value("CONSUMER_SECRET").toString());
    ui->defaultCallbackURLLineEdit->setText(settings->value("CALLBACK_URL").toString());

}

void MainWindow::saveSettings()
{
    settings->setValue("USERNAME", ui->userNameLineEdit->text());
    settings->setValue("PASSWORD", ui->passwordLineEdit->text());
    settings->setValue("BLOG_IDENTIFIER", ui->blogNameLineEdit->text());
    settings->setValue("CONSUMER_KEY", ui->consumerKeyLineEdit->text());
    settings->setValue("CONSUMER_SECRET", ui->consumerSecretLineEdit->text());
    settings->setValue("CALLBACK_URL", ui->defaultCallbackURLLineEdit->text());
}

void MainWindow::on_pushButton_clicked()
{
    if (this->thread->isRunning()) {
        tumblrDownloaderWorker->running = false;
        this->thread->quit();
        ui->statusTextArea->appendPlainText("* Terminated...");
        std::cout << "* Terminated..." << std::endl;
        ui->pushButton->setText("Download");
    } else {
        saveSettings();
        thread->start();
        ui->pushButton->setText("Cancel");
    }
}

void MainWindow::receiveTumblrImageURL(const QString &imgURL)
{
    ui->statusTextArea->appendPlainText(imgURL);
    std::cout << imgURL.toStdString().c_str() << std::endl;
}

void MainWindow::receiveTumblrImageURLFinished()
{

}
