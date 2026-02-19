#ifndef STATSTAB_H
#define STATSTAB_H

#include "logentry.h"
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QRadioButton>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QHBoxLayout>
#include <QMouseEvent>

class StatsTab : public QWidget {
    Q_OBJECT

public:
    explicit StatsTab(const QString& mode, QWidget* parent = nullptr);

    void setData(const QVector<LogEntry>& entries);
    void startLiveUpdates(int intervalMs);
    void stopLiveUpdates();

    // Returns the number of entries currently displayed (post-filter count
    // of the total dataset, used by MainWindow for status bar updates).
    int entryCount() const { return m_allEntries.size(); }

signals:
    void needsRefresh();

private slots:
    void onFilterChanged();
    void onRowClicked(int row);
    void onCloseDetail();
    void onExportCSV();
    void onStatCardClicked(const QString& severity);
    void onChartPointClicked(const QString& chartType, const QVariant& data);

private:
    QString             m_mode;
    QVector<LogEntry>   m_allEntries;
    QVector<LogEntry>   m_filteredEntries;
    QTimer*             m_refreshTimer;

    // Stat cards (outer container widgets)
    QWidget* m_criticalCard;
    QWidget* m_errorCard;
    QWidget* m_warningCard;
    QWidget* m_threatsCard;
    QWidget* m_totalCard;

    // Value labels inside each card
    QLabel* m_criticalLabel;
    QLabel* m_errorLabel;
    QLabel* m_warningLabel;
    QLabel* m_threatsLabel;
    QLabel* m_totalLabel;

    QChartView* m_timelineChart;
    QChartView* m_donutChart;
    QChartView* m_unitsChart;

    QRadioButton* m_filterAll;
    QRadioButton* m_filterCritical;
    QRadioButton* m_filterError;
    QRadioButton* m_filterWarning;
    QRadioButton* m_filterThreats;
    QComboBox*    m_unitFilter;
    QLineEdit*    m_searchBox;
    QLabel*       m_rowCountLabel;

    QTableWidget* m_table;
    QWidget*      m_detailPanel;
    QTextEdit*    m_detailContent;

    void setupUI();
    QHBoxLayout* createStatCards();
    void createCharts();
    QHBoxLayout* createFilters();
    void createTable();
    void createDetailPanel();

    void updateStats();
    void updateCharts();
    void updateTable();
    void updateUnitFilter();
    void showDetail(const LogEntry& entry);

    void applyFilters();

    // createStatCard installs an event filter on the card widget.
    // Child label/value widgets are made transparent to mouse events via
    // setAttribute(Qt::WA_TransparentForMouseEvents) so every click anywhere
    // on the card surface — including on the text — routes to the card's
    // own event filter handler.
    QWidget* createStatCard(const QString& icon, const QString& label,
                            const QString& value, const QColor& color);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
};

#endif // STATSTAB_H
