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

MainWindow::MainWindow(int lookbackDays, int liveWindowMinutes, int livePollSeconds, QWidget* parent)
    : QMainWindow(parent)
    , m_lookbackDays(lookbackDays)
    , m_liveWindowMinutes(liveWindowMinutes)
    , m_livePollSeconds(livePollSeconds)
    , m_scanThread(new QThread(this))
    , m_liveThread(new QThread(this))
{
    setWindowTitle("Error Surface");
    resize(1400, 900);
    
    setupUI();
}

MainWindow::~MainWindow() {
    m_scanThread->quit();
    m_liveThread->quit();
    m_scanThread->wait();
    m_liveThread->wait();
}

void MainWindow::setupUI() {
    // Central widget
    auto* central = new QWidget();
    setCentralWidget(central);
    
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Header
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
    
    // Scan days control
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
    
    // Live window control
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
    
    // Live poll control
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
    auto* refreshBtn = new QPushButton("🔄 Refresh Scan");
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        m_statusLabel->setText("Refreshing scan...");
        QApplication::processEvents();
        auto entries = m_scanCollector->collectAll(m_lookbackDays);
        m_scanTab->setData(entries);
        m_statusLabel->setText(QString("Scan refreshed · %1 entries").arg(entries.size()));
        m_tabs->setTabText(0, QString("◉  SCAN  —  %1d historical").arg(m_lookbackDays));
    });
    headerLayout->addWidget(refreshBtn);
    
    headerLayout->addSpacing(20);
    
    m_statusLabel = new QLabel("Initializing...");
    m_statusLabel->setStyleSheet(R"(
        font-family: 'JetBrains Mono', monospace;
        font-size: 11px;
        color: #555;
        border: none;
        padding: 0;
        background: transparent;
    )");
    headerLayout->addWidget(m_statusLabel);
    
    mainLayout->addWidget(header);
    
    // Tabs
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
                              .arg(m_liveWindowMinutes)
                              .arg(m_livePollSeconds));
    
    mainLayout->addWidget(m_tabs);
    
    // Apply dark theme
    qApp->setStyle("Fusion");
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor("#0d0d0f"));
    darkPalette.setColor(QPalette::WindowText, QColor("#c8c8d4"));
    darkPalette.setColor(QPalette::Base, QColor("#13131a"));
    darkPalette.setColor(QPalette::AlternateBase, QColor("#0d0d0f"));
    darkPalette.setColor(QPalette::Text, QColor("#c8c8d4"));
    darkPalette.setColor(QPalette::Button, QColor("#13131a"));
    darkPalette.setColor(QPalette::ButtonText, QColor("#c8c8d4"));
    darkPalette.setColor(QPalette::Highlight, QColor("#7B61FF"));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    qApp->setPalette(darkPalette);
}

void MainWindow::startCollections() {
    m_statusLabel->setText("Collecting scan data...");
    QApplication::processEvents();
    
    // Use QTimer to defer collection so UI can render first
    QTimer::singleShot(100, this, [this]() {
        qDebug() << "Collecting scan data...";
        m_scanCollector = new LogCollector(this);
        auto scanEntries = m_scanCollector->collectAll(m_lookbackDays);
        qDebug() << "Scan collected:" << scanEntries.size();
        m_scanTab->setData(scanEntries);
        
        // Set up live collector in background thread
        m_liveCollector = new LogCollector();
        m_liveCollector->moveToThread(m_liveThread);
        
        // Connect live refresh signal to collection in thread
        connect(m_liveTab, &StatsTab::needsRefresh, this, [this]() {
            QMetaObject::invokeMethod(m_liveCollector, [this]() {
                auto entries = m_liveCollector->collectLive(m_liveWindowMinutes);
                QMetaObject::invokeMethod(this, [this, entries]() {
                    m_liveTab->setData(entries);
                    m_statusLabel->setText(QString("Live · %1 entries").arg(entries.size()));
                }, Qt::QueuedConnection);
            }, Qt::QueuedConnection);
        });
        
        m_liveThread->start();
        emit m_liveTab->needsRefresh();
        m_liveTab->startLiveUpdates(m_livePollSeconds * 1000);
        
        m_statusLabel->setText(QString("Ready · Scan: %1").arg(scanEntries.size()));
    });
}

void MainWindow::onScanRefresh() {
    // Not used with current implementation
}

void MainWindow::onLiveRefresh() {
    // Handled by lambda in startCollections
}

void MainWindow::onCollectionComplete(int count) {
    m_statusLabel->setText(QString("%1 entries loaded").arg(count));
}

void MainWindow::onCollectionError(const QString& error) {
    m_statusLabel->setText(QString("Error: %1").arg(error));
}
