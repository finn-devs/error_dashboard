#include "settingsdrawer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFrame>
#include <QScrollArea>
#include <QApplication>
#include <QScreen>
#include <QDebug>

static constexpr int kDrawerWidth   = 360;
static constexpr int kAnimDurationMs = 250;

SettingsDrawer::SettingsDrawer(PersistenceManager* persistence, QWidget* parent)
    : QWidget(parent)
    , m_persistence(persistence)
    , m_animation(new QPropertyAnimation(this, "drawerX", this))
{
    // Draw on top of everything else in the parent
    setWindowFlags(Qt::Widget);
    setAttribute(Qt::WA_StyledBackground, true);

    setFixedWidth(kDrawerWidth);
    setStyleSheet(R"(
        SettingsDrawer {
            background: #13131a;
            border-left: 1px solid #7B61FF;
        }
        QLabel {
            color: #c8c8d4;
            font-family: 'JetBrains Mono', monospace;
            font-size: 11px;
            background: transparent;
        }
        QComboBox, QLineEdit, QSpinBox {
            background: #0d0d0f;
            color: #c8c8d4;
            border: 1px solid #22222e;
            border-radius: 5px;
            padding: 5px 10px;
            font-family: 'JetBrains Mono', monospace;
            font-size: 11px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QPushButton {
            background: none;
            border: 1px solid #22222e;
            color: #888;
            font-family: 'JetBrains Mono', monospace;
            font-size: 10px;
            padding: 6px 14px;
            border-radius: 5px;
        }
        QPushButton:hover {
            border-color: #7B61FF;
            color: #7B61FF;
        }
        QPushButton#dangerBtn {
            border-color: #FF2D55;
            color: #FF2D55;
        }
        QPushButton#dangerBtn:hover {
            background: #1a0008;
        }
        QPushButton#primaryBtn {
            background: #7B61FF;
            color: white;
            border: none;
        }
        QPushButton#primaryBtn:hover {
            background: #9078FF;
        }
    )");

    m_animation->setDuration(kAnimDurationMs);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    setupUI();

    // Start hidden (off-screen to the right)
    if (parent) {
        move(parent->width(), 0);
        resize(kDrawerWidth, parent->height());
    }
    raise();
}

// ---------------------------------------------------------------------------
// Q_PROPERTY helpers for geometry animation
// ---------------------------------------------------------------------------

int SettingsDrawer::drawerX() const {
    return x();
}

void SettingsDrawer::setDrawerX(int xPos) {
    move(xPos, 0);
    if (parentWidget()) resize(kDrawerWidth, parentWidget()->height());
}

// ---------------------------------------------------------------------------
// Open / Close
// ---------------------------------------------------------------------------

void SettingsDrawer::slideOpen() {
    if (m_open) return;
    m_open = true;

    if (parentWidget()) resize(kDrawerWidth, parentWidget()->height());

    refreshDbStats();
    show();
    raise();

    const int target = parentWidget() ? parentWidget()->width() - kDrawerWidth : 0;
    m_animation->stop();
    m_animation->setStartValue(parentWidget() ? parentWidget()->width() : x());
    m_animation->setEndValue(target);
    m_animation->start();
}

void SettingsDrawer::slideClose() {
    if (!m_open) return;
    m_open = false;

    const int offscreen = parentWidget() ? parentWidget()->width() : x() + kDrawerWidth + 10;
    m_animation->stop();
    m_animation->setStartValue(x());
    m_animation->setEndValue(offscreen);
    connect(m_animation, &QPropertyAnimation::finished, this, [this]() {
        hide();
        // Disconnect so it doesn't fire again on the next open/close cycle
        disconnect(m_animation, &QPropertyAnimation::finished, this, nullptr);
    });
    m_animation->start();
}

bool SettingsDrawer::isDrawerOpen() const {
    return m_open;
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

QWidget* SettingsDrawer::makeSectionHeader(const QString& title) {
    auto* w = new QWidget();
    auto* layout = new QVBoxLayout(w);
    layout->setContentsMargins(0, 16, 0, 6);
    layout->setSpacing(4);

    auto* label = new QLabel(title);
    label->setStyleSheet(R"(
        font-size: 9px;
        font-weight: 700;
        letter-spacing: 2px;
        color: #555;
        text-transform: uppercase;
    )");
    layout->addWidget(label);

    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background: #22222e; border: none; max-height: 1px;");
    layout->addWidget(line);

    return w;
}

QWidget* SettingsDrawer::makeRow(QWidget* labelWidget, QWidget* control) {
    auto* row = new QWidget();
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 4, 0, 4);
    layout->addWidget(labelWidget, 1);
    layout->addWidget(control, 2);
    return row;
}

void SettingsDrawer::setupUI() {
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // ---- Panel header ----
    auto* header = new QWidget();
    header->setStyleSheet("background: #0d0d0f; border-bottom: 1px solid #22222e; padding: 0;");
    header->setFixedHeight(52);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 0, 16, 0);

    auto* titleLabel = new QLabel("SETTINGS");
    titleLabel->setStyleSheet(R"(
        font-size: 12px;
        font-weight: 700;
        letter-spacing: 3px;
        color: #c8c8d4;
    )");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    auto* closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(28, 28);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            font-size: 14px;
            border: none;
            color: #555;
            padding: 0;
        }
        QPushButton:hover { color: #c8c8d4; }
    )");
    connect(closeBtn, &QPushButton::clicked, this, &SettingsDrawer::closeRequested);
    headerLayout->addWidget(closeBtn);

    outerLayout->addWidget(header);

    // ---- Scrollable content area ----
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: #13131a; border: none; }");

    auto* content = new QWidget();
    content->setStyleSheet("background: #13131a;");
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(20, 8, 20, 20);
    contentLayout->setSpacing(2);

    // ===========================
    // Section: Data & Retention
    // ===========================
    contentLayout->addWidget(makeSectionHeader("Data & Retention"));

    // TTL selector
    auto* ttlLabel = new QLabel("Log retention (TTL)");
    m_ttlCombo = new QComboBox();
    m_ttlCombo->addItem("7 days",   7);
    m_ttlCombo->addItem("14 days",  14);
    m_ttlCombo->addItem("30 days",  30);
    m_ttlCombo->addItem("60 days",  60);
    m_ttlCombo->addItem("90 days",  90);
    m_ttlCombo->addItem("1 year",  365);
    m_ttlCombo->addItem("Custom…",  -1);

    // Pre-select current TTL
    for (int i = 0; i < m_ttlCombo->count(); ++i) {
        if (m_ttlCombo->itemData(i).toInt() == m_persistence->ttlDays()) {
            m_ttlCombo->setCurrentIndex(i);
            break;
        }
    }

    connect(m_ttlCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDrawer::onTtlComboChanged);
    contentLayout->addWidget(makeRow(ttlLabel, m_ttlCombo));

    // Custom TTL spin (hidden unless "Custom…" is selected)
    m_customTtlRow = new QWidget();
    {
        auto* rowLayout = new QHBoxLayout(m_customTtlRow);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        auto* customLabel = new QLabel("Custom days");
        m_customTtlSpin = new QSpinBox();
        m_customTtlSpin->setRange(1, 3650);
        m_customTtlSpin->setValue(m_persistence->ttlDays());
        m_customTtlSpin->setSuffix(" days");
        connect(m_customTtlSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &SettingsDrawer::onCustomTtlChanged);
        rowLayout->addWidget(customLabel, 1);
        rowLayout->addWidget(m_customTtlSpin, 2);
    }
    m_customTtlRow->setVisible(false);
    contentLayout->addWidget(m_customTtlRow);

    // TTL note
    auto* ttlNote = new QLabel("TTL applies to newly stored events only.\nExisting records keep their original expiry.");
    ttlNote->setWordWrap(true);
    ttlNote->setStyleSheet("font-size: 10px; color: #444; padding: 4px 0;");
    contentLayout->addWidget(ttlNote);

    contentLayout->addSpacing(6);

    // Purge expired now
    auto* purgeBtn = new QPushButton("⌫  Purge Expired Now");
    connect(purgeBtn, &QPushButton::clicked, this, &SettingsDrawer::onPurgeNow);
    contentLayout->addWidget(purgeBtn);

    contentLayout->addSpacing(4);

    // Clear all data
    auto* clearBtn = new QPushButton("⚠  Clear All Data");
    clearBtn->setObjectName("dangerBtn");
    connect(clearBtn, &QPushButton::clicked, this, &SettingsDrawer::onClearAll);
    contentLayout->addWidget(clearBtn);

    // ===========================
    // Section: Database
    // ===========================
    contentLayout->addWidget(makeSectionHeader("Database"));

    // DB path
    auto* pathLabel = new QLabel("Database path");
    m_dbPathEdit = new QLineEdit();
    m_dbPathEdit->setPlaceholderText("~/.local/share/error-surface/events.db");
    m_dbPathEdit->setText(m_persistence->currentPath());
    contentLayout->addWidget(makeRow(pathLabel, m_dbPathEdit));

    // Browse + Apply row
    {
        auto* btnRow = new QWidget();
        auto* btnLayout = new QHBoxLayout(btnRow);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->addStretch();
        auto* browseBtn = new QPushButton("Browse…");
        connect(browseBtn, &QPushButton::clicked, this, &SettingsDrawer::onBrowseDb);
        btnLayout->addWidget(browseBtn);
        auto* applyBtn = new QPushButton("Apply Path");
        applyBtn->setObjectName("primaryBtn");
        connect(applyBtn, &QPushButton::clicked, this, &SettingsDrawer::onApplyPath);
        btnLayout->addWidget(applyBtn);
        contentLayout->addWidget(btnRow);
    }

    contentLayout->addSpacing(6);

    // DB stats
    m_dbSizeLabel = new QLabel("Size: —");
    m_dbSizeLabel->setStyleSheet("font-size: 10px; color: #555;");
    contentLayout->addWidget(m_dbSizeLabel);

    m_dbEventCountLabel = new QLabel("Events stored: —");
    m_dbEventCountLabel->setStyleSheet("font-size: 10px; color: #555;");
    contentLayout->addWidget(m_dbEventCountLabel);

    contentLayout->addStretch();

    scroll->setWidget(content);
    outerLayout->addWidget(scroll);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void SettingsDrawer::onTtlComboChanged(int index) {
    const int days = m_ttlCombo->itemData(index).toInt();
    if (days == -1) {
        // Custom
        m_customTtlRow->setVisible(true);
        m_persistence->setTtlDays(m_customTtlSpin->value());
        emit ttlChanged(m_customTtlSpin->value());
    } else {
        m_customTtlRow->setVisible(false);
        m_persistence->setTtlDays(days);
        emit ttlChanged(days);
    }
}

void SettingsDrawer::onCustomTtlChanged(int value) {
    m_persistence->setTtlDays(value);
    emit ttlChanged(value);
}

void SettingsDrawer::onBrowseDb() {
    const QString path = QFileDialog::getSaveFileName(
        this,
        "Select Database File",
        m_dbPathEdit->text(),
        "SQLite Database (*.db);;All Files (*)"
    );
    if (!path.isEmpty()) {
        m_dbPathEdit->setText(path);
    }
}

void SettingsDrawer::onApplyPath() {
    const QString path = m_dbPathEdit->text().trimmed();
    if (!path.isEmpty()) {
        emit dbPathChanged(path);
    }
}

void SettingsDrawer::onPurgeNow() {
    emit purgeRequested();
    refreshDbStats();
}

void SettingsDrawer::onClearAll() {
    QMessageBox confirm(this);
    confirm.setWindowTitle("Clear All Data");
    confirm.setText("This will permanently delete all stored log events.");
    confirm.setInformativeText("This action cannot be undone. Continue?");
    confirm.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    confirm.setDefaultButton(QMessageBox::Cancel);
    confirm.setStyleSheet(R"(
        QMessageBox { background: #13131a; color: #c8c8d4; }
        QPushButton { background: #13131a; color: #c8c8d4; border: 1px solid #22222e;
                      padding: 6px 16px; border-radius: 5px; }
        QPushButton:hover { border-color: #7B61FF; }
    )");

    if (confirm.exec() == QMessageBox::Yes) {
        emit clearAllRequested();
        refreshDbStats();
    }
}

// ---------------------------------------------------------------------------
// Stats refresh
// ---------------------------------------------------------------------------

void SettingsDrawer::refreshDbStats() {
    if (!m_persistence->isOpen()) {
        m_dbSizeLabel->setText("Size: not connected");
        m_dbEventCountLabel->setText("Events stored: —");
        return;
    }

    const qint64 bytes = m_persistence->databaseSizeBytes();
    QString sizeStr;
    if (bytes < 1024)           sizeStr = QString("%1 B").arg(bytes);
    else if (bytes < 1048576)   sizeStr = QString("%1 KB").arg(bytes / 1024);
    else                        sizeStr = QString("%1 MB").arg(bytes / 1048576);

    m_dbSizeLabel->setText(QString("Size: %1").arg(sizeStr));

    // Event count comes from the active (non-expired) set
    const int count = m_persistence->loadActiveEvents().size();
    m_dbEventCountLabel->setText(QString("Events stored: %1").arg(count));
}
