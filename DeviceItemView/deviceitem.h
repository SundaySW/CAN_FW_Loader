#ifndef DEVICEITEM_H
#define DEVICEITEM_H

#include <QGroupBox>

namespace Ui {
class DeviceItem;
}

class DeviceItem : public QGroupBox
{
    Q_OBJECT

public:
    explicit DeviceItem(QWidget *parent = nullptr);
    ~DeviceItem();

private:
    Ui::DeviceItem *ui;
};

#endif // DEVICEITEM_H
