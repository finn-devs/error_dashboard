#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "statstab.h"
#include "logcollector.h"
#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QThread>
#include <QSpinBox>

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(int lookbackDays = 7, int liveWindowMinutes = 60, 
                       int livePollSeconds = 5, QWidget* parent = nullptr);
    ~MainWindow();
    
    void startCollections();
    
private slots:
    void onScanRefresh();
    void onLiveRefresh();
    void onCollectionComplete(int count);
    void onCollectionError(const QString& error);
    
private:
    int m_lookbackDays;
    int m_liveWindowMinutes;
    int m_livePollSeconds;
    
    QTabWidget* m_tabs;
    StatsTab* m_scanTab;
    StatsTab* m_liveTab;
    QLabel* m_statusLabel;
    
    LogCollector* m_scanCollector;
    LogCollector* m_liveCollector;
    QThread* m_scanThread;
    QThread* m_liveThread;
    
    void setupUI();
};

#endif // MAINWINDOW_H
