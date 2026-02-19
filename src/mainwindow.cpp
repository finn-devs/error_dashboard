#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QApplication>
#include <QStyle>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QDir>
#include <QStandardPaths>
#include <QSet>
#include <QDebug>

// ---------------------------------------------------------------------------
// XDG default path
// ---------------------------------------------------------------------------

QString MainWindow::defaultDbPath() {
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);
    return dataDir + "/events.db";
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

MainWindow::MainWindow(int lookbackDays, int liveWindowMinutes, int livePollSeconds, QWidget* parent)
    : QMainWindow(parent)
    , m_lookbackDays(lookbackDays)
    , m_liveWindowMinutes(liveWindowMinutes)
    , m_livePollSeconds(livePollSeconds)
    , m_scanThread(new QThread(this))
    , m_liveThread(new QThread(this))
    , m_persistence(new PersistenceManager(this))
    , m_settingsDrawer(nullptr)  // constructed after setupUI so parent geometry is known
{
    setWindowTitle("Error Surface");
    resize(1400, 900);

    setupUI();

    // Now that the window geometry exists, build the settings drawer
    // It is a child of the central widget so it overlays the content area.
    m_settingsDrawer = new SettingsDrawer(m_persistence, centralWidget());

    connect(m_settingsDrawer, &SettingsDrawer::closeRequested,   this, [this]() { m_settingsDrawer->slideClose(); });
    connect(m_settingsDrawer, &SettingsDrawer::dbPathChanged,    this, &MainWindow::onDbPathChanged);
    connect(m_settingsDrawer, &SettingsDrawer::ttlChanged,       this, &MainWindow::onTtlChanged);
    connect(m_settingsDrawer, &SettingsDrawer::purgeRequested,   this, &MainWindow::onPurgeRequested);
    connect(m_settingsDrawer, &SettingsDrawer::clearAllRequested,this, &MainWindow::onClearAllRequested);

    // Open the database at the default XDG path
    const QString dbPath = defaultDbPath();
    if (!m_persistence->open(dbPath)) {
        qWarning() << "Could not open persistence database at" << dbPath;
    } else {
        qDebug() << "Database opened at" << dbPath;
        // Purge any records whose TTL has expired
        const int purged = m_persistence->purgeExpired();
        if (purged > 0) qDebug() << "Purged" << purged << "expired records on startup";
    }
}

MainWindow::~MainWindow() {
    m_scanThread->quit();
    m_liveThread->quit();
    m_scanThread->wait();
    m_liveThread->wait();
}

// ---------------------------------------------------------------------------
// resizeEvent — keep the settings drawer pinned to the right edge
// ---------------------------------------------------------------------------

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (m_settingsDrawer) {
        m_settingsDrawer->resize(m_settingsDrawer->width(), centralWidget()->height());
        if (m_settingsDrawer->isDrawerOpen()) {
            m_settingsDrawer->move(centralWidget()->width() - m_settingsDrawer->width(), 0);
        } else {
            m_settingsDrawer->move(centralWidget()->width(), 0);
        }
    }
}

// ---------------------------------------------------------------------------
// UI Construction
// ---------------------------------------------------------------------------

void MainWindow::setupUI() {
    auto* central = new QWidget();
    setCentralWidget(central);

    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---- Header ----
    auto* header = new QWidget();
    header->setStyleSheet(R"(
        QWidget {
            background: #13131a;
            border-bottom: 1px solid #22222e;
            padding: 16px 24px;
        }
        QSpinBox, QLabel {
            background: #0d0d0f;
            color: #c8c8d4;
            border: 1px solid #22222e;
            border-radius: 6px;
            padding: 4px 8px;
            font-family: 'JetBrains Mono', monospace;
            font-size: 11px;
        }
        QPushButton {
            background: #7B61FF;
            color: white;
            border: none;
            padding: 6px 16px;
            border-radius: 6px;
            font-family: 'JetBrains Mono', monospace;
            font-size: 11px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #9078FF;
        }
        QPushButton#gearBtn {
            background: none;
            border: 1px solid #22222e;
            color: #888;
            font-size: 16px;
            padding: 4px 10px;
            border-radius: 6px;
        }
        QPushButton#gearBtn:hover {
            border-color: #7B61FF;
            color: #7B61FF;
        }
    )");

    auto* headerLayout = new QHBoxLayout(header);

    auto* title = new QLabel("ERROR SURFACE");
    title->setStyleSheet(R"(
        font-family: 'JetBrains Mono', monospace;
        font-size: 20px;
        font-weight: 700;
        color: #ffffff;
        letter-spacing: 3px;
        border: none;
        padding: 0;
    )");
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    // Scan days
    auto* daysLabel = new QLabel("Scan Days:");
    daysLabel->setStyleSheet("border: none; background: transparent; padding: 0;");
    headerLayout->addWidget(daysLabel);
    auto* daysSpinBox = new QSpinBox();
    daysSpinBox->setRange(1, 30);
    daysSpinBox->setValue(m_lookbackDays);
    daysSpinBox->setMinimumWidth(60);
    connect(daysSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int days) {
        m_lookbackDays = days;
    });
    headerLayout->addWidget(daysSpinBox);
    headerLayout->addSpacing(20);

    // Live window
    auto* liveLabel = new QLabel("Live Window (min):");
    liveLabel->setStyleSheet("border: none; background: transparent; padding: 0;");
    headerLayout->addWidget(liveLabel);
    auto* liveWindowSpinBox = new QSpinBox();
    liveWindowSpinBox->setRange(5, 240);
    liveWindowSpinBox->setValue(m_liveWindowMinutes);
    liveWindowSpinBox->setMinimumWidth(60);
    connect(liveWindowSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int mins) {
        m_liveWindowMinutes = mins;
    });
    headerLayout->addWidget(liveWindowSpinBox);
    headerLayout->addSpacing(20);

    // Poll interval
    auto* pollLabel = new QLabel("Poll Interval (s):");
    pollLabel->setStyleSheet("border: none; background: transparent; padding: 0;");
    headerLayout->addWidget(pollLabel);
    auto* pollSpinBox = new QSpinBox();
    pollSpinBox->setRange(1, 60);
    pollSpinBox->setValue(m_livePollSeconds);
    pollSpinBox->setMinimumWidth(60);
    connect(pollSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int secs) {
        m_livePollSeconds = secs;
        m_liveTab->stopLiveUpdates();
        m_liveTab->startLiveUpdates(secs * 1000);
    });
    headerLayout->addWidget(pollSpinBox);
    headerLayout->addSpacing(20);

    // Refresh button
    auto* refreshBtn = new QPushButton("↻ Refresh Scan");
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        m_statusLabel->setText("Refreshing scan…");
        QApplication::processEvents();
        auto entries = m_scanCollector->collectAll(m_lookbackDays);
        mergeAndDisplay(entries);
        m_statusLabel->setText(QString("Scan refreshed · %1 entries").arg(m_scanTab->entryCount()));
        m_tabs->setTabText(0, QString("◉  SCAN  —  %1d historical").arg(m_lookbackDays));
    });
    headerLayout->addWidget(refreshBtn);
    headerLayout->addSpacing(12);

    // Status label
    m_statusLabel = new QLabel("Initializing…");
    m_statusLabel->setStyleSheet(R"(
        font-family: 'JetBrains Mono', monospace;
        font-size: 11px;
        color: #555;
        border: none;
        padding: 0;
        background: transparent;
    )");
    headerLayout->addWidget(m_statusLabel);
    headerLayout->addSpacing(12);

    // Gear icon (upper right)
    auto* gearBtn = new QPushButton("⚙");
    gearBtn->setObjectName("gearBtn");
    gearBtn->setToolTip("Settings");
    connect(gearBtn, &QPushButton::clicked, this, &MainWindow::onGearClicked);
    headerLayout->addWidget(gearBtn);

    mainLayout->addWidget(header);

    // ---- Tabs ----
    m_tabs = new QTabWidget();
    m_tabs->setStyleSheet(R"(
        QTabWidget::pane {
            background: #0d0d0f;
            border: none;
        }
        QTabBar::tab {
            background: #0d0d0f;
            color: #666;
            font-family: 'JetBrains Mono', monospace;
            font-size: 11px;
            letter-spacing: 1px;
            padding: 10px 20px;
            border: 1px solid #22222e;
            border-bottom: none;
        }
        QTabBar::tab:selected {
            background: #13131a;
            color: #7B61FF;
            border-bottom: 2px solid #7B61FF;
        }
    )");

    m_scanTab = new StatsTab("scan");
    m_liveTab = new StatsTab("live");

    m_tabs->addTab(m_scanTab, QString("◉  SCAN  —  %1d historical").arg(m_lookbackDays));
    m_tabs->addTab(m_liveTab, QString("●  LIVE  —  %1min / %2s poll")
                              .arg(m_liveWindowMinutes).arg(m_livePollSeconds));

    mainLayout->addWidget(m_tabs);

    // Dark theme
    qApp->setStyle("Fusion");
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window,          QColor("#0d0d0f"));
    darkPalette.setColor(QPalette::WindowText,      QColor("#c8c8d4"));
    darkPalette.setColor(QPalette::Base,            QColor("#13131a"));
    darkPalette.setColor(QPalette::AlternateBase,   QColor("#0d0d0f"));
    darkPalette.setColor(QPalette::Text,            QColor("#c8c8d4"));
    darkPalette.setColor(QPalette::Button,          QColor("#13131a"));
    darkPalette.setColor(QPalette::ButtonText,      QColor("#c8c8d4"));
    darkPalette.setColor(QPalette::Highlight,       QColor("#7B61FF"));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    qApp->setPalette(darkPalette);
}

// ---------------------------------------------------------------------------
// startCollections — startup sequence
// ---------------------------------------------------------------------------

void MainWindow::startCollections() {
    // Step 1: Load persisted events immediately so the dashboard is populated
    // before the first scan completes.
    if (m_persistence->isOpen()) {
        const auto persisted = m_persistence->loadActiveEvents();
        if (!persisted.isEmpty()) {
            m_scanTab->setData(persisted);
            m_statusLabel->setText(QString("Loaded %1 stored events · Scanning…").arg(persisted.size()));
        }
    }

    // Step 2: Defer the actual journal scan so the UI can render first
    QTimer::singleShot(150, this, [this]() {
        qDebug() << "Starting journal scan…";
        m_scanCollector = new LogCollector(this);
        const auto freshEntries = m_scanCollector->collectAll(m_lookbackDays);
        qDebug() << "Scan collected:" << freshEntries.size() << "entries";

        mergeAndDisplay(freshEntries);

        // Step 3: Set up live collector
        m_liveCollector = new LogCollector();
        m_liveCollector->moveToThread(m_liveThread);

        connect(m_liveTab, &StatsTab::needsRefresh, this, [this]() {
            QMetaObject::invokeMethod(m_liveCollector, [this]() {
                const auto entries = m_liveCollector->collectLive(m_liveWindowMinutes);
                QMetaObject::invokeMethod(this, [this, entries]() {
                    m_liveTab->setData(entries);
                    m_statusLabel->setText(QString("Live · %1 entries").arg(entries.size()));
                }, Qt::QueuedConnection);
            }, Qt::QueuedConnection);
        });

        m_liveThread->start();
        emit m_liveTab->needsRefresh();
        m_liveTab->startLiveUpdates(m_livePollSeconds * 1000);

        m_statusLabel->setText(QString("Ready · Scan: %1").arg(m_scanTab->entryCount()));
    });
}

// ---------------------------------------------------------------------------
// mergeAndDisplay
// ---------------------------------------------------------------------------

void MainWindow::mergeAndDisplay(const QVector<LogEntry>& freshEntries) {
    if (!m_persistence->isOpen()) {
        // No persistence — just show fresh entries directly
        m_scanTab->setData(freshEntries);
        return;
    }

    // Persist the new entries (upsert — duplicates are silently ignored)
    m_persistence->upsertEvents(freshEntries);

    // Load the full non-expired set from the DB (this naturally includes
    // both the freshly inserted events and all previously stored events).
    // Because every event has a unique fingerprint and we upserted before
    // loading, there are no duplicates in this result set.
    const auto merged = m_persistence->loadActiveEvents();
    m_scanTab->setData(merged);
}

// ---------------------------------------------------------------------------
// Gear / Settings slots
// ---------------------------------------------------------------------------

void MainWindow::onGearClicked() {
    if (m_settingsDrawer->isDrawerOpen()) {
        m_settingsDrawer->slideClose();
    } else {
        m_settingsDrawer->slideOpen();
    }
}

void MainWindow::onDbPathChanged(const QString& newPath) {
    // Re-open the database at the new path, carrying over the current TTL.
    const int currentTtl = m_persistence->ttlDays();
    if (m_persistence->open(newPath)) {
        m_persistence->setTtlDays(currentTtl);
        m_persistence->purgeExpired();
        m_statusLabel->setText("Database path updated.");
        // Reload from new DB
        const auto persisted = m_persistence->loadActiveEvents();
        m_scanTab->setData(persisted);
    } else {
        m_statusLabel->setText("Failed to open database at new path.");
    }
    m_settingsDrawer->refreshDbStats();
}

void MainWindow::onTtlChanged(int days) {
    // setTtlDays is already called inside SettingsDrawer before this signal
    // fires, so all we need to do here is update the status label.
    m_statusLabel->setText(QString("TTL set to %1 days (applies to new events)").arg(days));
}

void MainWindow::onPurgeRequested() {
    const int removed = m_persistence->purgeExpired();
    m_statusLabel->setText(QString("Purged %1 expired records.").arg(removed));
    // Reload so the table reflects the purge
    const auto updated = m_persistence->loadActiveEvents();
    m_scanTab->setData(updated);
}

void MainWindow::onClearAllRequested() {
    m_persistence->clearAll();
    m_scanTab->setData({});
    m_statusLabel->setText("All stored data cleared.");
}

// ---------------------------------------------------------------------------
// Unused stubs (kept for signal compatibility)
// ---------------------------------------------------------------------------

void MainWindow::onScanRefresh() {}
void MainWindow::onLiveRefresh() {}
void MainWindow::onCollectionComplete(int count) {
    m_statusLabel->setText(QString("%1 entries loaded").arg(count));
}
void MainWindow::onCollectionError(const QString& error) {
    m_statusLabel->setText(QString("Error: %1").arg(error));
}
