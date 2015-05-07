#ifndef BANKNODEMANAGER_H
#define BANKNODEMANAGER_H

#include "util.h"
#include "sync.h"

#include <QWidget>
#include <QTimer>

namespace Ui {
    class BanknodeManager;
}
class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Banknode Manager page widget */
class BanknodeManager : public QWidget
{
    Q_OBJECT

public:
    explicit BanknodeManager(QWidget *parent = 0);
    ~BanknodeManager();



public slots:
    void updateNodeList();
    void updateAdrenalineNode(QString alias, QString addr, QString privkey, QString collateral);

signals:

private:
    QTimer *timer;
    Ui::BanknodeManager *ui;

    CCriticalSection cs_adrenaline;
    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

private slots:
    void on_copyAddressButton_clicked();
    void on_createButton_clicked();
    void on_editButton_clicked();
    void on_getConfigButton_clicked();
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_startAllButton_clicked();
    void on_stopAllButton_clicked();
    void on_removeButton_clicked();
    void on_tableWidget_2_itemSelectionChanged();
};

#endif // BANKNODEMANAGER_H
