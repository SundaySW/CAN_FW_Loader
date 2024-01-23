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
    void ProcessFinished(qint64 msec);
    void PostDeviceData(uint uid, uchar addr, uchar hw, uchar fw);
    void SetReqData(const QString &fileName, uchar addr, uchar version);
    ~DeviceItem() override;
signals:
    void RefreshDevice();
    void CancelDevice();
private:
    Ui::DeviceItem *ui;
};

#endif // DEVICEITEM_H
