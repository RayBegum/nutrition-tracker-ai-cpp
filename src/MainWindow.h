#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ClaudeClient.h"
#include "Tracker.h"
#include <QMainWindow>
#include <memory>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTableWidget;

// The app window. Data is handled by Tracker; food analysis is done by ClaudeClient.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    // Adds foods to today's log under the given meal (used after the user confirms).
    void addFoods(const QVector<ParsedFood>& items, const QString& meal);

private slots:
    void analyzeFood();
    void onAnalysis(const AnalysisResult& result);
    bool setApiKey();
    void addManual();
    void deleteSelected();
    void goalsChanged();

private:
    QWidget* buildTodayTab();
    QWidget* buildManualTab();
    QWidget* buildHistoryTab();
    void refresh();
    void setBar(QProgressBar* bar, QLabel* label, double value, double goal, const QString& unit, int digits);
    QString apiKey() const;
    static QString todayDate();

    std::unique_ptr<Tracker> tracker_;
    ClaudeClient* client_ = nullptr;
    bool updatingGoals_ = false;
    QString pendingMeal_;

    // "What did you eat?"
    QLineEdit* sentence_ = nullptr;
    QComboBox* mealAI_ = nullptr;
    QPushButton* analyzeBtn_ = nullptr;
    QLabel* status_ = nullptr;

    // goals
    QSpinBox* calGoal_ = nullptr;
    QDoubleSpinBox* proGoal_ = nullptr;
    QDoubleSpinBox* fibGoal_ = nullptr;

    // progress
    QProgressBar *calBar_ = nullptr, *proBar_ = nullptr, *fibBar_ = nullptr;
    QLabel *calText_ = nullptr, *proText_ = nullptr, *fibText_ = nullptr;
    QLabel* sugarFat_ = nullptr;

    // manual entry
    QLineEdit* name_ = nullptr;
    QComboBox* meal_ = nullptr;
    QSpinBox* cal_ = nullptr;
    QDoubleSpinBox *pro_ = nullptr, *fib_ = nullptr, *sugar_ = nullptr, *sat_ = nullptr;

    QTableWidget* foodTable_ = nullptr;
    QTableWidget* historyTable_ = nullptr;
    QLabel* dateLabel_ = nullptr;
};

#endif
