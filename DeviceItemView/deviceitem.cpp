#include "deviceitem.h"
#include "ui_deviceitem.h"

DeviceItem::DeviceItem(QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::DeviceItem)
{
    ui->setupUi(this);
    connect(ui->refresh_btn, &QPushButton::clicked, this, [this]{ emit RefreshDevice(); });
    ui->progressBar->setValue(false);
}

DeviceItem::~DeviceItem()
{
    delete ui;
}

void DeviceItem::ProcessStarted() {
    ui->progressBar->setValue(0);
    ui->progressBar->setVisible(true);
}

void DeviceItem::ProcessFinished(int msec) {
    ui->elapsed_label->setText(tr("%1 msec").arg(msec));
    ui->refresh_btn->setEnabled(false);
    ui->cancel_btn->setEnabled(false);
}

void DeviceItem::SetStatusValue(int delta) {
    ui->progressBar->setValue(delta);
}
