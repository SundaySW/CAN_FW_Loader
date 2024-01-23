#include "deviceitem.h"
#include "ui_deviceitem.h"

DeviceItem::DeviceItem(QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::DeviceItem)
{
    ui->setupUi(this);
    ui->upload_btn->setEnabled(false);
    ui->cancel_btn->setEnabled(false);
    ui->time_label->setVisible(false);
    connect(ui->upload_btn, &QPushButton::clicked, this, [this]{ emit RefreshDevice(); });
    connect(ui->cancel_btn, &QPushButton::clicked, this, [this]{
        ui->upload_btn->setEnabled(false);
        ui->cancel_btn->setEnabled(false);
        emit CancelDevice();
    });
    ui->progressBar->setValue(false);
}

DeviceItem::~DeviceItem()
{
    delete ui;
}

void DeviceItem::ProcessStarted() {
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
    ui->upload_btn->setEnabled(true);
    ui->cancel_btn->setEnabled(true);
}

void DeviceItem::ProcessFinished(qint64 msec) {
    ui->progressBar->setValue(100);
    ui->elapsed_label->setText(tr("%1 msec").arg(msec));
    ui->upload_btn->setEnabled(false);
    ui->cancel_btn->setEnabled(false);
}

void DeviceItem::SetStatusValue(int delta) {
    ui->progressBar->setValue(delta);
}

void DeviceItem::PostDeviceData(uint uid, uchar addr, uchar hw, uchar fw) {
    ui->uid_label->setText(QString("0x%1").arg(uid,1,16));
    ui->addr_label->setText(QString("0x%1").arg(addr,1,16));
    ui->hw_ver_label->setText(QString("%1").arg(hw));
    ui->sw_ver_label->setText(QString("%1").arg(fw));
}

void DeviceItem::SetReqData(const QString& fileName, uchar addr, uchar sw_version) {
    ui->file_name_label->setText(fileName);
    ui->req_addr_label->setText(QString("New addr: 0x%1").arg(addr,1,16));
    ui->req_sw_ver_label->setText(QString("New SW Version: %1").arg(sw_version));
}