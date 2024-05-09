#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "Sftp/transfersftp.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init();

    setWindowTitle("Mtr1994");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init()
{
    connect(ui->btnDownload, &QPushButton::clicked, this, &MainWindow::slot_btn_download_click);
    connect(ui->btnUpload, &QPushButton::clicked, this, &MainWindow::slot_btn_upload_click);

    connect(TransferSftp::getInstance(), &TransferSftp::sgl_file_upload_process, this, &MainWindow::slot_file_upload_process);
    connect(TransferSftp::getInstance(), &TransferSftp::sgl_file_download_process, this, &MainWindow::slot_file_download_process);

    ui->widgetProgressUpload->setColor(QColor(103, 194, 58));
}

void MainWindow::slot_btn_download_click()
{
    TransferSftp::getInstance()->download("/Upload/DeskArcGIS/ABC.zip", "./BCD.zip");
}

void MainWindow::slot_btn_upload_click()
{
    TransferSftp::getInstance()->upload(QString("./Upload/%1").arg("geoserver-2.23.1-war.zip"), QString("/Upload/DeskArcGIS/%1").arg("ABC.zip"));
    return;
    for (int i = 1; i < 10; i++)
    {
        QString fileName = QString("U%1.rar").arg(QString::number(i));
        TransferSftp::getInstance()->upload(QString("./Upload/%1").arg(fileName), QString("/Upload/DeskArcGIS/%1").arg(fileName));
    }
}

void MainWindow::slot_file_upload_process(const QString &local, uint16_t rate)
{
    Q_UNUSED(local);
    ui->widgetProgressUpload->setValue(rate / 100.0);
    if (rate == 10000) qDebug() << "file upload finish " << local;
}

void MainWindow::slot_file_download_process(const QString &remote, uint16_t rate)
{
    Q_UNUSED(remote);
    ui->widgetProgressDownload->setValue(rate / 100.0);
    if (rate == 10000) qDebug() << "file download finish " << remote;
}

