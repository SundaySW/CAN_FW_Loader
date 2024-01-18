#include "deviceitem.h"
#include "ui_deviceitem.h"

DeviceItem::DeviceItem(QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::DeviceItem)
{
    ui->setupUi(this);
}

DeviceItem::~DeviceItem()
{
    delete ui;
}
