#ifndef SETTINGSDRAWER_H
#define SETTINGSDRAWER_H

#include "persistencemanager.h"
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QSpinBox>

class SettingsDrawer : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int drawerX READ drawerX WRITE setDrawerX)

public:
    explicit SettingsDrawer(PersistenceManager* persistence, QWidget* parent = nullptr);

    void slideOpen();
    void slideClose();
    bool isDrawerOpen() const;

    // Sync the displayed DB stats (called after purge, load, etc.)
    void refreshDbStats();

signals:
    void dbPathChanged(const QString& newPath);
    void ttlChanged(int days);
    void purgeRequested();
    void clearAllRequested();
    void closeRequested();

private slots:
    void onTtlComboChanged(int index);
    void onCustomTtlChanged(int value);
    void onBrowseDb();
    void onApplyPath();
    void onPurgeNow();
    void onClearAll();

private:
    // Q_PROPERTY helpers for animation
    int  drawerX() const;
    void setDrawerX(int x);

    void setupUI();
    QWidget* makeSectionHeader(const QString& title);
    QWidget* makeRow(QWidget* label, QWidget* control);

    PersistenceManager* m_persistence;
    QPropertyAnimation* m_animation;
    bool m_open = false;

    // Controls
    QComboBox* m_ttlCombo;
    QSpinBox*  m_customTtlSpin;
    QWidget*   m_customTtlRow;
    QLineEdit* m_dbPathEdit;
    QLabel*    m_dbSizeLabel;
    QLabel*    m_dbEventCountLabel;
};

#endif // SETTINGSDRAWER_H
