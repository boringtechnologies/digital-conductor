//*****************************************************************************
//*****************************************************************************

#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QQmlEngine>

#include <memory>

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

    Q_PROPERTY(QString newDeviceLicense READ generateLicenseToken CONSTANT)

protected:
    AppController();

public:
    static AppController * instance(QQmlEngine * = nullptr, QJSEngine * = nullptr);
    virtual ~AppController();

public:
    State   state() const;

    QString generateLicenseToken() const;

signals:
    void showMessage(const QString & text);
    void stateChanged();

public slots:
    void init(const QString & userName, const QString & group, const QString & target);

    void registerLicense(const QString license);
    void checkLicense();

    void copyToClipboard(QString data);

private:
    class impl;
    std::shared_ptr<impl> m_p;
};

#endif // APPCONTROLLER_H
