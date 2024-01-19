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
    void SetStatusValue(int delta);
    void ProcessStarted();
    void ProcessFinished(int msec);
    ~DeviceItem();
signals:
    void RefreshDevice();
private:
    Ui::DeviceItem *ui;
};

#endif // DEVICEITEM_H
