//*****************************************************************************
//*****************************************************************************

#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include "autodiscovery.grpc.pb.h"
#include "core/media/util.h"

#include <QObject>
#include <QQmlEngine>

#include <memory>

//*****************************************************************************
//*****************************************************************************
namespace dc
{
class GRpcClient;
}

//*****************************************************************************
//*****************************************************************************
class AppController : public QObject
{
    Q_OBJECT

  public:
    enum State
    {
        StateFirst,
        StateWelcome = StateFirst,
        StateRegisterLicense,
        StateCompleted,
    };
    Q_ENUM(State)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)

    Q_PROPERTY(QString group READ group CONSTANT)
    Q_PROPERTY(QString newDeviceLicense READ generateLicenseToken CONSTANT)

    Q_PROPERTY(bool syncModeEnabled READ syncModeEnabled WRITE setSyncMode NOTIFY syncModeChanged)

  public:
    static AppController *instance(QQmlEngine * = nullptr, QJSEngine * = nullptr);
    virtual ~AppController();

    // std::shared_ptr<dc::GRpcClient> rpcClient();
    std::vector<dc::Peer> getPeers() const;

    State state() const;

    QString group() const;

    QString generateLicenseToken() const;

    bool syncModeEnabled() const;

  signals:
    void showMessage(const QString &text);
    void stateChanged();
    void syncModeChanged();
    void setSenderBusy(bool busy);
    void setReceiverBusy(bool busy);
    void setGroupEditable(bool editable);
    void updatedGroup(QString name);

  public slots:
    void init(const QString &userName, const QString &group, const QString &target, const QString &rwMode);

    void registerLicense(const QString license);
    void checkLicense();

    void copyToClipboard(QString data);

    void setSyncMode(const bool sync);

    void selectDevice(const dc::media::DeviceDescr &dev);

    void startStreaming();
    void receiveStream(bool recreate = true);
    void stopStream();
    void stopReceive();

    void onSenderViewReady();

    void groupChanged(QString newGroup);

  protected:
    AppController();

    void updateGroupEditable();

  private:
    class impl;
    std::shared_ptr<impl> m_p;
};

#endif // APPCONTROLLER_H
