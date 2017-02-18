#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "tumblrdownloaderworker.h"

#include <QThread>
#include <iostream>

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

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_blogNameLineEdit_textChanged(const QString &arg1)
{

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
