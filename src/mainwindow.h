#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "statstab.h"
#include "logcollector.h"
#include "persistencemanager.h"
#include "settingsdrawer.h"
#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QThread>
#include <QSpinBox>
#include <QPushButton>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(int lookbackDays = 7, int liveWindowMinutes = 60,
                       int livePollSeconds = 5, QWidget* parent = nullptr);
    ~MainWindow();

    void startCollections();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onScanRefresh();
    void onLiveRefresh();
    void onCollectionComplete(int count);
    void onCollectionError(const QString& error);

    void onGearClicked();
    void onDbPathChanged(const QString& newPath);
    void onTtlChanged(int days);
    void onPurgeRequested();
    void onClearAllRequested();

private:
    int m_lookbackDays;
    int m_liveWindowMinutes;
    int m_livePollSeconds;

    QTabWidget* m_tabs;
    StatsTab*   m_scanTab;
    StatsTab*   m_liveTab;
    QLabel*     m_statusLabel;

    LogCollector* m_scanCollector;
    LogCollector* m_liveCollector;
    QThread*      m_scanThread;
    QThread*      m_liveThread;

    PersistenceManager* m_persistence;
    SettingsDrawer*     m_settingsDrawer;

    void setupUI();

    // Merged view: persisted events + freshly-scanned events, no duplicates.
    // The fingerprint set is used to deduplicate; fresh entries that already
    // exist in the DB are not double-shown.
    void mergeAndDisplay(const QVector<LogEntry>& freshEntries);

    // Default XDG-compliant DB path (~/.local/share/error-surface/events.db)
    static QString defaultDbPath();
};

#endif // MAINWINDOW_H
