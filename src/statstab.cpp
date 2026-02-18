#include "statstab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QEvent>
#include <QMouseEvent>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QHorizontalBarSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

StatsTab::StatsTab(const QString& mode, QWidget* parent)
    : QWidget(parent), m_mode(mode), m_refreshTimer(new QTimer(this))
{
    setupUI();
    connect(m_refreshTimer, &QTimer::timeout, this, &StatsTab::needsRefresh);
}

void StatsTab::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);
    
    // Apply dark theme stylesheet
    setStyleSheet(R"(
        QWidget {
            background-color: #0d0d0f;
            color: #c8c8d4;
            font-family: 'DM Sans', sans-serif;
        }
        QLabel {
            color: #c8c8d4;
        }
        QTableWidget {
            background-color: #13131a;
            border: 1px solid #22222e;
            border-radius: 10px;
            gridline-color: #22222e;
        }
        QTableWidget::item {
            padding: 7px 12px;
            border-bottom: 1px solid #22222e;
        }
        QTableWidget::item:selected {
            background-color: #1e1e3a;
            border: 1px solid #7B61FF;
        }
        QHeaderView::section {
            background-color: #09090c;
            color: #555;
            font-family: 'JetBrains Mono', monospace;
            font-size: 9px;
            font-weight: bold;
            text-transform: uppercase;
            letter-spacing: 1px;
            padding: 10px 12px;
            border: 1px solid #22222e;
            border-bottom: 2px solid #22222e;
        }
        QRadioButton {
            font-family: 'JetBrains Mono', monospace;
            font-size: 12px;
            spacing: 8px;
        }
        QRadioButton::indicator {
            width: 14px;
            height: 14px;
        }
        QRadioButton::indicator:checked {
            background-color: #7B61FF;
            border: 2px solid #7B61FF;
            border-radius: 7px;
        }
        QComboBox, QLineEdit {
            background-color: #0d0d0f;
            color: #c8c8d4;
            border: 1px solid #22222e;
            border-radius: 6px;
            padding: 6px 12px;
            font-family: 'JetBrains Mono', monospace;
            font-size: 11px;
        }
        QPushButton {
            background: none;
            border: 1px solid #22222e;
            color: #888;
            font-family: 'JetBrains Mono', monospace;
            font-size: 10px;
            padding: 5px 12px;
            border-radius: 6px;
        }
        QPushButton:hover {
            border-color: #7B61FF;
            color: #7B61FF;
        }
    )");
    
    auto* statsLayout = createStatCards();
    mainLayout->addLayout(statsLayout);
    
    createCharts();
    auto* chartsLayout = new QHBoxLayout();
    chartsLayout->addWidget(m_timelineChart, 2);
    chartsLayout->addWidget(m_donutChart, 1);
    chartsLayout->addWidget(m_unitsChart, 1);
    mainLayout->addLayout(chartsLayout);
    
    auto* filtersLayout = createFilters();
    mainLayout->addLayout(filtersLayout);
    
    createDetailPanel();
    mainLayout->addWidget(m_detailPanel);
    m_detailPanel->setVisible(false);
    
    createTable();
    mainLayout->addWidget(m_table);
}

QHBoxLayout* StatsTab::createStatCards() {
    auto* layout = new QHBoxLayout();
    layout->setSpacing(12);
    
    m_criticalLabel = new QLabel();
    m_errorLabel = new QLabel();
    m_warningLabel = new QLabel();
    m_threatsLabel = new QLabel();
    m_totalLabel = new QLabel();
    
    m_criticalCard = createStatCard("⛔", "Critical", "0", QColor("#FF2D55"));
    m_errorCard = createStatCard("🔴", "Error", "0", QColor("#FF6B35"));
    m_warningCard = createStatCard("⚠️", "Warning", "0", QColor("#FFD60A"));
    m_threatsCard = createStatCard("🛡", "Threats", "0", QColor("#FF0055"));
    m_totalCard = createStatCard("∑", "Total", "0", QColor("#7B61FF"));
    
    layout->addWidget(m_criticalCard);
    layout->addWidget(m_errorCard);
    layout->addWidget(m_warningCard);
    layout->addWidget(m_threatsCard);
    layout->addWidget(m_totalCard);
    
    return layout;
}

QWidget* StatsTab::createStatCard(const QString& icon, const QString& label, const QString& value, const QColor& color) {
    auto* card = new QWidget();
    card->setStyleSheet(QString(R"(
        QWidget {
            background: #13131a;
            border: 1px solid #22222e;
            border-radius: 10px;
            padding: 18px 22px;
        }
        QWidget:hover {
            background: #1a1a22;
            border-color: #7B61FF;
            cursor: pointer;
        }
    )"));
    card->setCursor(Qt::PointingHandCursor);
    card->setProperty("severity", label.toLower());
    
    // Install event filter for click handling
    card->installEventFilter(this);
    
    auto* layout = new QVBoxLayout(card);
    
    auto* labelWidget = new QLabel(QString("%1 %2").arg(icon, label));
    labelWidget->setStyleSheet(R"(
        font-size: 11px;
        color: #666;
        letter-spacing: 1px;
        text-transform: uppercase;
        font-family: 'JetBrains Mono', monospace;
    )");
    
    auto* valueWidget = new QLabel(value);
    valueWidget->setStyleSheet(QString(R"(
        font-size: 32px;
        font-weight: 700;
        color: %1;
        font-family: 'JetBrains Mono', monospace;
    )").arg(color.name()));
    
    // Store reference based on label
    if (label == "Critical") m_criticalLabel = valueWidget;
    else if (label == "Error") m_errorLabel = valueWidget;
    else if (label == "Warning") m_warningLabel = valueWidget;
    else if (label == "Threats") m_threatsLabel = valueWidget;
    else if (label == "Total") m_totalLabel = valueWidget;
    
    layout->addWidget(labelWidget);
    layout->addWidget(valueWidget);
    layout->addStretch();
    
    return card;
}

void StatsTab::createCharts() {
    //using namespace QtCharts;
    
    // Timeline chart (left panel)
    m_timelineChart = new QChartView();
    m_timelineChart->setRenderHint(QPainter::Antialiasing);
    m_timelineChart->chart()->setBackgroundBrush(QBrush(QColor("#0d0d0f")));
    m_timelineChart->chart()->setPlotAreaBackgroundBrush(QBrush(QColor("#13131a")));
    m_timelineChart->chart()->legend()->setVisible(false);
    m_timelineChart->chart()->setTitle("Error Timeline");
    m_timelineChart->chart()->setTitleBrush(QBrush(QColor("#c8c8d4")));
    m_timelineChart->setMinimumHeight(220);
    
    // Insights chart (center panel)
    m_donutChart = new QChartView();
    m_donutChart->setRenderHint(QPainter::Antialiasing);
    m_donutChart->chart()->setBackgroundBrush(QBrush(QColor("#0d0d0f")));
    m_donutChart->chart()->setPlotAreaBackgroundBrush(QBrush(QColor("#13131a")));
    m_donutChart->chart()->legend()->setVisible(true);
    m_donutChart->chart()->legend()->setAlignment(Qt::AlignRight);
    m_donutChart->chart()->legend()->setLabelColor(QColor("#c8c8d4"));
    m_donutChart->chart()->setTitle("Threat Severity");
    m_donutChart->chart()->setTitleBrush(QBrush(QColor("#c8c8d4")));
    m_donutChart->setMinimumHeight(220);
    
    // Top units chart (right panel)
    m_unitsChart = new QChartView();
    m_unitsChart->setRenderHint(QPainter::Antialiasing);
    m_unitsChart->chart()->setBackgroundBrush(QBrush(QColor("#0d0d0f")));
    m_unitsChart->chart()->setPlotAreaBackgroundBrush(QBrush(QColor("#13131a")));
    m_unitsChart->chart()->legend()->setVisible(false);
    m_unitsChart->chart()->setTitle("Top Problem Units");
    m_unitsChart->chart()->setTitleBrush(QBrush(QColor("#c8c8d4")));
    m_unitsChart->setMinimumHeight(220);
}

QHBoxLayout* StatsTab::createFilters() {
    auto* layout = new QHBoxLayout();
    layout->setSpacing(14);
    
    // Severity filter
    auto* sevLabel = new QLabel("Severity:");
    sevLabel->setStyleSheet("font-size: 10px; color: #555; text-transform: uppercase;");
    layout->addWidget(sevLabel);
    
    m_filterAll = new QRadioButton("All");
    m_filterCritical = new QRadioButton("⛔ Critical");
    m_filterError = new QRadioButton("🔴 Error");
    m_filterWarning = new QRadioButton("⚠️ Warning");
    m_filterThreats = new QRadioButton("🛡 Threats");
    m_filterAll->setChecked(true);
    
    for (auto* btn : {m_filterAll, m_filterCritical, m_filterError, m_filterWarning, m_filterThreats}) {
        layout->addWidget(btn);
        connect(btn, &QRadioButton::clicked, this, &StatsTab::onFilterChanged);
    }
    
    // Unit filter
    layout->addWidget(new QLabel(" | "));
    auto* unitLabel = new QLabel("Unit:");
    unitLabel->setStyleSheet("font-size: 10px; color: #555;");
    layout->addWidget(unitLabel);
    
    m_unitFilter = new QComboBox();
    m_unitFilter->addItem("All units", "all");
    m_unitFilter->setMinimumWidth(220);
    connect(m_unitFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatsTab::onFilterChanged);
    layout->addWidget(m_unitFilter);
    
    // Search
    layout->addWidget(new QLabel(" | "));
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Search message, unit, exe…");
    m_searchBox->setMinimumWidth(280);
    connect(m_searchBox, &QLineEdit::textChanged, this, &StatsTab::onFilterChanged);
    layout->addWidget(m_searchBox);
    
    layout->addStretch();
    
    // Row count + export
    m_rowCountLabel = new QLabel("0 rows");
    m_rowCountLabel->setStyleSheet("font-size: 11px; color: #555;");
    layout->addWidget(m_rowCountLabel);
    
    auto* exportBtn = new QPushButton("⬇ Export CSV");
    connect(exportBtn, &QPushButton::clicked, this, &StatsTab::onExportCSV);
    layout->addWidget(exportBtn);
    return layout;
}

void StatsTab::createTable() {
    m_table = new QTableWidget();
    m_table->setColumnCount(11);
    m_table->setHorizontalHeaderLabels({
        "Timestamp", "🛡", "Severity", "P", "Source", "Unit / Service",
        "PID", "Executable", "Host", "Boot", "Message"
    });
    
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(true);
    
    // Set column widths
    m_table->setColumnWidth(0, 165); // Timestamp
    m_table->setColumnWidth(1, 60);  // Threat
    m_table->setColumnWidth(2, 110); // Severity
    m_table->setColumnWidth(3, 30);  // P
    m_table->setColumnWidth(4, 80);  // Source
    m_table->setColumnWidth(5, 180); // Unit
    m_table->setColumnWidth(6, 60);  // PID
    m_table->setColumnWidth(7, 120); // Executable
    m_table->setColumnWidth(8, 90);  // Host
    m_table->setColumnWidth(9, 75);  // Boot
    
    connect(m_table, &QTableWidget::cellClicked, this, &StatsTab::onRowClicked);
}

void StatsTab::createDetailPanel() {
    m_detailPanel = new QWidget();
    m_detailPanel->setStyleSheet(R"(
        QWidget {
            background: #0f0f1a;
            border: 1px solid #7B61FF;
            border-radius: 10px;
            padding: 18px 22px;
        }
    )");
    
    auto* layout = new QVBoxLayout(m_detailPanel);
    
    auto* header = new QHBoxLayout();
    auto* title = new QLabel("EVENT DETAIL");
    title->setStyleSheet("font-size: 10px; color: #555; font-weight: 700; letter-spacing: 2px;");
    header->addWidget(title);
    header->addStretch();
    
    auto* closeBtn = new QPushButton("✕ Close");
    connect(closeBtn, &QPushButton::clicked, this, &StatsTab::onCloseDetail);
    header->addWidget(closeBtn);
    
    layout->addLayout(header);
    
    m_detailContent = new QTextEdit();
    m_detailContent->setReadOnly(true);
    m_detailContent->setStyleSheet(R"(
        background: #0a0a10;
        border: 1px solid #22222e;
        border-radius: 6px;
        padding: 14px;
        font-family: 'JetBrains Mono', monospace;
        font-size: 11px;
    )");
    layout->addWidget(m_detailContent);
}

void StatsTab::setData(const QVector<LogEntry>& entries) {
    m_allEntries = entries;
    updateStats();
    updateCharts();
    updateUnitFilter();
    applyFilters();
}

void StatsTab::updateStats() {
    int critical = 0, error = 0, warning = 0, threats = 0;
    for (const auto& entry : m_allEntries) {
        if (entry.group == "critical") critical++;
        else if (entry.group == "error") error++;
        else if (entry.group == "warning") warning++;
        threats += entry.threatCount;
    }
    
    m_criticalLabel->setText(QString::number(critical));
    m_errorLabel->setText(QString::number(error));
    m_warningLabel->setText(QString::number(warning));
    m_threatsLabel->setText(QString::number(threats));
    m_totalLabel->setText(QString::number(m_allEntries.size()));
}

void StatsTab::updateCharts() {
    //using namespace QtCharts;
    
    // === LEFT: Timeline Chart ===
    m_timelineChart->chart()->removeAllSeries();
    for (auto* axis : m_timelineChart->chart()->axes()) {
        m_timelineChart->chart()->removeAxis(axis);
    }
    
    // Group entries by time period (hourly for live, daily for scan)
    QMap<QString, QVector<int>> timeBuckets; // key -> [critical, error, warning]
    
    for (const auto& entry : m_allEntries) {
        QString bucket;
        if (m_mode == "live") {
            bucket = entry.timestamp.toString("hh:00");
        } else {
            bucket = entry.timestamp.toString("MM-dd");
        }
        
        if (!timeBuckets.contains(bucket)) {
            timeBuckets[bucket] = {0, 0, 0};
        }
        
        if (entry.group == "critical") timeBuckets[bucket][0]++;
        else if (entry.group == "error") timeBuckets[bucket][1]++;
        else if (entry.group == "warning") timeBuckets[bucket][2]++;
    }
    
    auto* criticalSeries = new QBarSet("Critical");
    auto* errorSeries = new QBarSet("Error");
    auto* warningSeries = new QBarSet("Warning");
    
    criticalSeries->setColor(QColor("#FF2D55"));
    errorSeries->setColor(QColor("#FF6B35"));
    warningSeries->setColor(QColor("#FFD60A"));
    
    QStringList categories;
    for (auto it = timeBuckets.begin(); it != timeBuckets.end(); ++it) {
        categories << it.key();
        *criticalSeries << it.value()[0];
        *errorSeries << it.value()[1];
        *warningSeries << it.value()[2];
    }
    
    auto* barSeries = new QStackedBarSeries();
    barSeries->append(criticalSeries);
    barSeries->append(errorSeries);
    barSeries->append(warningSeries);
    
    m_timelineChart->chart()->addSeries(barSeries);
    
    auto* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(QColor("#888"));
    m_timelineChart->chart()->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    
    auto* axisY = new QValueAxis();
    axisY->setLabelsColor(QColor("#888"));
    m_timelineChart->chart()->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
    
    // === CENTER: Threat Severity Pie Chart ===
    m_donutChart->chart()->removeAllSeries();
    
    QMap<QString, int> threatCounts;
    for (const auto& entry : m_allEntries) {
        if (entry.threatCount > 0) {
            QString severity = entry.maxThreatSeverity.isEmpty() ? "unknown" : entry.maxThreatSeverity;
            threatCounts[severity]++;
        }
    }
    
    if (!threatCounts.isEmpty()) {
        auto* pieSeries = new QPieSeries();
        
        for (auto it = threatCounts.begin(); it != threatCounts.end(); ++it) {
            auto* slice = pieSeries->append(it.key(), it.value());
            
            if (it.key() == "critical") slice->setColor(QColor("#FF2D55"));
            else if (it.key() == "high") slice->setColor(QColor("#FF6B35"));
            else if (it.key() == "medium") slice->setColor(QColor("#FFD60A"));
            else slice->setColor(QColor("#7B61FF"));
            
            slice->setLabelColor(QColor("#c8c8d4"));
            slice->setLabelVisible(true);
        }
        
        pieSeries->setHoleSize(0.4);
        m_donutChart->chart()->addSeries(pieSeries);
    } else {
        // Show "No threats detected" placeholder
        auto* pieSeries = new QPieSeries();
        auto* slice = pieSeries->append("No threats", 1);
        slice->setColor(QColor("#2a2a2a"));
        slice->setLabelColor(QColor("#666"));
        m_donutChart->chart()->addSeries(pieSeries);
    }
    
    // === RIGHT: Top Problem Units ===
    m_unitsChart->chart()->removeAllSeries();
    for (auto* axis : m_unitsChart->chart()->axes()) {
        m_unitsChart->chart()->removeAxis(axis);
    }
    
    QMap<QString, int> unitCounts;
    for (const auto& entry : m_allEntries) {
        if (!entry.unit.isEmpty() && entry.unit != "unknown") {
            unitCounts[entry.unit]++;
        }
    }
    
    // Get top 5
    QList<QPair<int, QString>> sorted;
    for (auto it = unitCounts.begin(); it != unitCounts.end(); ++it) {
        sorted.append({it.value(), it.key()});
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });
    
    if (!sorted.isEmpty()) {
        auto* barSet = new QBarSet("Errors");
        barSet->setColor(QColor("#FF6B35"));
        
        QStringList units;
        for (int i = 0; i < qMin(5, sorted.size()); ++i) {
            *barSet << sorted[i].first;
            QString unitName = sorted[i].second;
            if (unitName.length() > 25) unitName = unitName.left(22) + "...";
            units << unitName;
        }
        
        auto* barSeries = new QHorizontalBarSeries();
        barSeries->append(barSet);
        m_unitsChart->chart()->addSeries(barSeries);
        
        auto* axisY = new QBarCategoryAxis();
        axisY->append(units);
        axisY->setLabelsColor(QColor("#888"));
        m_unitsChart->chart()->addAxis(axisY, Qt::AlignLeft);
        barSeries->attachAxis(axisY);
        
        auto* axisX = new QValueAxis();
        axisX->setLabelsColor(QColor("#888"));
        m_unitsChart->chart()->addAxis(axisX, Qt::AlignBottom);
        barSeries->attachAxis(axisX);
    }
}

void StatsTab::updateUnitFilter() {
    QString current = m_unitFilter->currentData().toString();
    m_unitFilter->clear();
    m_unitFilter->addItem("All units", "all");
    
    QSet<QString> units;
    for (const auto& entry : m_allEntries) {
        if (!entry.unit.isEmpty()) units.insert(entry.unit);
    }
    
    for (const QString& unit : units) {
        m_unitFilter->addItem(unit, unit);
    }
    
    int idx = m_unitFilter->findData(current);
    if (idx >= 0) m_unitFilter->setCurrentIndex(idx);
}

void StatsTab::applyFilters() {
    m_filteredEntries.clear();
    
    QString groupFilter = "all";
    if (m_filterCritical->isChecked()) groupFilter = "critical";
    else if (m_filterError->isChecked()) groupFilter = "error";
    else if (m_filterWarning->isChecked()) groupFilter = "warning";
    else if (m_filterThreats->isChecked()) groupFilter = "threats";
    
    QString unitFilter = m_unitFilter->currentData().toString();
    QString search = m_searchBox->text().toLower();
    
    for (const auto& entry : m_allEntries) {
        // Group filter
        if (groupFilter != "all") {
            if (groupFilter == "threats") {
                if (entry.threatCount == 0) continue;
            } else if (entry.group != groupFilter) {
                continue;
            }
        }
        
        // Unit filter
        if (unitFilter != "all" && entry.unit != unitFilter) continue;
        
        // Search filter
        if (!search.isEmpty()) {
            if (!entry.message.toLower().contains(search) &&
                !entry.unit.toLower().contains(search) &&
                !entry.exe.toLower().contains(search) &&
                !entry.cmdline.toLower().contains(search)) {
                continue;
            }
        }
        
        m_filteredEntries.append(entry);
    }
    
    updateTable();
}

void StatsTab::updateTable() {
    m_table->setRowCount(qMin(m_filteredEntries.size(), 2000));
    
    for (int i = 0; i < m_table->rowCount(); ++i) {
        const auto& entry = m_filteredEntries[i];
        
        m_table->setItem(i, 0, new QTableWidgetItem(entry.timestamp.toString("yyyy-MM-dd HH:mm:ss UTC")));
        m_table->setItem(i, 1, new QTableWidgetItem(entry.threatBadge()));
        m_table->setItem(i, 2, new QTableWidgetItem(entry.severityLabel()));
        m_table->setItem(i, 3, new QTableWidgetItem(QString::number(entry.priority)));
        m_table->setItem(i, 4, new QTableWidgetItem(entry.source));
        m_table->setItem(i, 5, new QTableWidgetItem(entry.unit));
        m_table->setItem(i, 6, new QTableWidgetItem(entry.pid));
        
        QString exeBase = entry.exe.section('/', -1);
        m_table->setItem(i, 7, new QTableWidgetItem(exeBase));
        m_table->setItem(i, 8, new QTableWidgetItem(entry.hostname));
        m_table->setItem(i, 9, new QTableWidgetItem(entry.bootId));
        m_table->setItem(i, 10, new QTableWidgetItem(entry.message.left(300)));
        
        // Set row colors based on severity
        for (int col = 0; col < m_table->columnCount(); ++col) {
            auto* item = m_table->item(i, col);
            item->setBackground(QBrush(entry.severityBgColor()));
            item->setForeground(QBrush(entry.severityColor()));
        }
    }
    
    m_rowCountLabel->setText(QString("%1 rows (of %2 total)")
                             .arg(m_filteredEntries.size())
                             .arg(m_allEntries.size()));
}

void StatsTab::onFilterChanged() {
    applyFilters();
}

void StatsTab::onStatCardClicked(const QString& severity) {
    if (severity == "critical") m_filterCritical->setChecked(true);
    else if (severity == "error") m_filterError->setChecked(true);
    else if (severity == "warning") m_filterWarning->setChecked(true);
    else if (severity == "threats") m_filterThreats->setChecked(true);
    else m_filterAll->setChecked(true);
    
    applyFilters();
}

void StatsTab::onChartPointClicked(const QString& chartType, const QVariant& data) {
    // Future: Handle chart clicks for filtering
    // e.g., click on a unit bar to filter to that unit
}

bool StatsTab::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget* widget = qobject_cast<QWidget*>(obj);
        if (widget) {
            QString severity = widget->property("severity").toString();
            if (!severity.isEmpty()) {
                onStatCardClicked(severity);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void StatsTab::onRowClicked(int row) {
    if (row < 0 || row >= m_filteredEntries.size()) return;
    showDetail(m_filteredEntries[row]);
}

void StatsTab::showDetail(const LogEntry& entry) {
    QString html = QString(R"(
        <div style='font-family: "JetBrains Mono"; font-size: 11px;'>
        <b>Timestamp:</b> %1<br>
        <b>Severity:</b> %2<br>
        <b>Priority:</b> P%3<br>
        <b>Unit:</b> %4<br>
        <b>PID:</b> %5<br>
        <b>Executable:</b> %6<br>
        <b>Command Line:</b><br><pre style='background:#0a0a10;padding:8px;'>%7</pre>
        <b>Full Message:</b><br><pre style='background:#0a0a10;padding:8px;border-left:4px solid %8;'>%9</pre>
    )").arg(entry.timestamp.toString("yyyy-MM-dd HH:mm:ss UTC"),
            entry.severityLabel(),
            QString::number(entry.priority),
            entry.unit,
            entry.pid,
            entry.exe,
            entry.cmdline,
            entry.severityColor().name(),
            entry.message);
    
    // Add threat details
    if (entry.threatCount > 0) {
        html += "<br><b style='color:#FF2D55;'>SECURITY THREATS DETECTED:</b><br>";
        for (const auto& threat : entry.threats) {
            html += QString("<div style='background:#1a0808;border:1px solid #FF2D5544;border-radius:6px;padding:10px;margin:8px 0;'>"
                           "<b style='color:#FF0055;'>🛡 %1</b> [%2]<br>"
                           "%3<br>"
                           "<small style='color:#555;'>Pattern: %4</small>"
                           "</div>")
                        .arg(threat.category, threat.severity.toUpper(), threat.description, threat.pattern);
        }
    }
    
    html += QString("<br><small style='color:#333;'>Cursor: %1</small></div>").arg(entry.cursor);
    
    m_detailContent->setHtml(html);
    m_detailPanel->setVisible(true);
}

void StatsTab::onCloseDetail() {
    m_detailPanel->setVisible(false);
}

void StatsTab::onExportCSV() {
    QString filename = QFileDialog::getSaveFileName(this, "Export CSV", 
                                                    QString("error_surface_%1_%2.csv")
                                                        .arg(m_mode)
                                                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
                                                    "CSV Files (*.csv)");
    if (filename.isEmpty()) return;
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    
    QTextStream out(&file);
    out << "Timestamp,Threats,Severity,Priority,Source,Unit,PID,Executable,Host,Boot,Message\n";
    
    for (const auto& entry : m_filteredEntries) {
        out << QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,\"%11\"\n")
                   .arg(entry.timestamp.toString("yyyy-MM-dd HH:mm:ss"),
                        entry.threatBadge(),
                        entry.severityLabel(),
                        QString::number(entry.priority),
                        entry.source,
                        entry.unit,
                        entry.pid,
                        entry.exe.section('/', -1),
                        entry.hostname,
                        entry.bootId,
                        QString(entry.message).replace("\"", "\"\""));
    }
}

void StatsTab::startLiveUpdates(int intervalMs) {
    m_refreshTimer->start(intervalMs);
}

void StatsTab::stopLiveUpdates() {
    m_refreshTimer->stop();
}
