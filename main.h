#ifndef MAIN_H
#define MAIN_H
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QApplication>

class MyObject : public QObject {
    Q_OBJECT
public:
    explicit MyObject(QApplication* application);
    void TestConnection() const;
    static void ReplyFinished(QNetworkReply *reply);

    QNetworkAccessManager *manager;
};
#endif // MAIN_H
