#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "tumblrdownloaderworker.h"

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

    thread = new QThread(this);
    tumblrDownloaderWorker = new TumblrDownloaderWorker();

    tumblrDownloaderWorker->moveToThread(thread);

    connect(thread, SIGNAL(started()), tumblrDownloaderWorker, SLOT(run()));
    connect(tumblrDownloaderWorker, SIGNAL(emitImageURL(QString)), this, SLOT(receiveTumblrImageURL(QString)));
    connect(tumblrDownloaderWorker, SIGNAL(receiveTumblrImageURLFinished()), this, SLOT(receiveTumblrImageURLFinished()) );

    loadSettings();

    ui->targetLocationLineEdit->installEventFilter(this);
    if (ui->targetLocationLineEdit->text().isEmpty()) {
        ui->targetLocationLineEdit->setText(QCoreApplication::applicationDirPath());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (this->thread->isRunning()) {
        tumblrDownloaderWorker->running = false;
        ui->pushButton->setText("Cancelling...");
        ui->statusTextArea->appendPlainText("* Cancelling...");
        this->thread->quit();
        qApp->processEvents();
        this->thread->wait();
    }

    saveSettings();
    event->accept();
}


void MainWindow::loadSettings()
{
    settings = new QSettings ("Tumblr Downloader", "Tumblr Downloader");

    ui->userNameLineEdit->setText(settings->value("USERNAME").toString());
    ui->passwordLineEdit->setText(settings->value("PASSWORD").toString());
    ui->blogNameLineEdit->setText(settings->value("BLOG_IDENTIFIER").toString());
    ui->consumerKeyLineEdit->setText(settings->value("CONSUMER_KEY").toString());
    ui->consumerSecretLineEdit->setText(settings->value("CONSUMER_SECRET").toString());
    //ui->defaultCallbackURLLineEdit->setText(settings->value("CALLBACK_URL").toString());
    ui->targetLocationLineEdit->setText(settings->value("TARGET_LOCATION").toString());
}

void MainWindow::saveSettings()
{
    settings->setValue("USERNAME", ui->userNameLineEdit->text());
    settings->setValue("PASSWORD", ui->passwordLineEdit->text());
    settings->setValue("BLOG_IDENTIFIER", ui->blogNameLineEdit->text());
    settings->setValue("CONSUMER_KEY", ui->consumerKeyLineEdit->text());
    settings->setValue("CONSUMER_SECRET", ui->consumerSecretLineEdit->text());
    //settings->setValue("CALLBACK_URL", "http://localhost/tumblr/callback");
    settings->setValue("TARGET_LOCATION", ui->targetLocationLineEdit->text());
    settings->sync();

    qputenv("USERNAME", ui->userNameLineEdit->text().toStdString().c_str());
    qputenv("PASSWORD", ui->passwordLineEdit->text().toStdString().c_str());
    qputenv("BLOG_IDENTIFIER", ui->blogNameLineEdit->text().toStdString().c_str());
    qputenv("CONSUMER_KEY", ui->consumerKeyLineEdit->text().toStdString().c_str());
    qputenv("CONSUMER_SECRET", ui->consumerSecretLineEdit->text().toStdString().c_str());
    qputenv("CALLBACK_URL", "http://localhost/tumblr/callback");
    qputenv("TARGET_LOCATION", ui->targetLocationLineEdit->text().toStdString().c_str());
}

void MainWindow::on_pushButton_clicked()
{
    if (this->thread->isRunning()) {
        tumblrDownloaderWorker->running = false;
        ui->pushButton->setText("Cancelling...");
        ui->statusTextArea->appendPlainText("* Cancelling...");
        this->thread->quit();
        qApp->processEvents();
        this->thread->wait();
        ui->pushButton->setText("Download");
    } else {
        saveSettings();
        thread->start();
        ui->pushButton->setText("Cancel");
        ui->statusTextArea->appendPlainText("* Starting...");
    }
}

void MainWindow::receiveTumblrImageURL(const QString &imgURL)
{
    ui->statusTextArea->appendPlainText(imgURL);
}

void MainWindow::receiveTumblrImageURLFinished()
{
    ui->statusTextArea->appendPlainText("* Finished...");
    ui->pushButton->setText("Download");
    this->tumblrDownloaderWorker->running = false;
    this->thread->quit();
    this->thread->wait();
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

