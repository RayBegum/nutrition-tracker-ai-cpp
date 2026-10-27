#include "MainWindow.h"

#include <QAction>
#include <QComboBox>
#include <QDate>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTime>
#include <QVBoxLayout>
#include <algorithm>

namespace {

const char* kStyle = R"(
QMainWindow, QWidget#page { background: #f4f7fb; }
QGroupBox { font-weight: bold; border: 1px solid #d5dde8; border-radius: 8px;
            margin-top: 12px; padding: 10px; background: white; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; color: #1f3a5f; }
QPushButton { background: #1f3a5f; color: white; border-radius: 6px; padding: 7px 16px; font-weight: bold; }
QPushButton:hover { background: #2d5086; }
QPushButton:disabled { background: #9aa8bd; }
QLineEdit#sentence { font-size: 15px; padding: 7px; }
QProgressBar { border: 1px solid #d5dde8; border-radius: 6px; background: #eef2f7; height: 18px; text-align: center; }
QProgressBar#cal::chunk { background: #f08c3a; border-radius: 5px; }
QProgressBar#pro::chunk { background: #3a8f5c; border-radius: 5px; }
QProgressBar#fib::chunk { background: #7a5cc4; border-radius: 5px; }
QTableWidget { background: white; gridline-color: #e3e8ef; }
QHeaderView::section { background: #1f3a5f; color: white; padding: 4px; border: none; }
)";

// Names must contain a letter and may not contain the | character (used in the save file).
bool validName(const QString& s) {
    bool letter = false;
    for (QChar c : s) if (c.isLetter()) letter = true;
    return letter && !s.contains('|');
}

QTableWidgetItem* cell(const QString& text, bool right = false) {
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    if (right) item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

QString one(double v) { return QString::number(v, 'f', 1); }

QString describe(const ParsedFood& f) {
    QString s = QString("%1\n    %2 kcal  |  protein %3 g  |  fiber %4 g  |  sugar %5 g  |  saturated fat %6 g")
                    .arg(f.name).arg(qRound(f.calories)).arg(one(f.protein)).arg(one(f.fiber))
                    .arg(one(f.sugar)).arg(one(f.satFat));
    if (!f.assumption.isEmpty()) s += "\n    Assumed: " + f.assumption;
    return s;
}

QString mealForNow() {
    int h = QTime::currentTime().hour();
    if (h < 11) return "Breakfast";
    if (h < 16) return "Lunch";
    if (h < 21) return "Dinner";
    return "Snack";
}

}  // namespace

QString MainWindow::todayDate() {
    return QDate::currentDate().toString("yyyy-MM-dd");
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    tracker_ = std::make_unique<Tracker>((dir + "/foods.txt").toStdString(),
                                         (dir + "/goals.txt").toStdString());
    tracker_->load();

    client_ = new ClaudeClient(this);
    connect(client_, &ClaudeClient::finished, this, &MainWindow::onAnalysis);

    setWindowTitle("Nutrition Goal Tracker");
    setStyleSheet(kStyle);

    QAction* keyAction = menuBar()->addMenu("Settings")->addAction("Set API key...");
    connect(keyAction, &QAction::triggered, this, [this]() { setApiKey(); });

    auto* tabs = new QTabWidget;
    tabs->addTab(buildTodayTab(), "Today");
    tabs->addTab(buildManualTab(), "Manual add");
    tabs->addTab(buildHistoryTab(), "History");
    setCentralWidget(tabs);
    resize(900, 860);

    refresh();
}

QString MainWindow::apiKey() const {
    QString key = QSettings().value("apiKey").toString().trimmed();
    if (key.isEmpty()) key = qEnvironmentVariable("ANTHROPIC_API_KEY").trimmed();
    return key;
}

bool MainWindow::setApiKey() {
    bool ok = false;
    QString key = QInputDialog::getText(
        this, "Claude API key",
        "Paste your Claude API key.\nIt is saved only on this computer, never in the code.",
        QLineEdit::Password, QString(), &ok).trimmed();
    if (!ok || key.isEmpty()) return false;
    QSettings().setValue("apiKey", key);
    return true;
}

QWidget* MainWindow::buildTodayTab() {
    auto* page = new QWidget;
    page->setObjectName("page");
    auto* layout = new QVBoxLayout(page);

    dateLabel_ = new QLabel;
    dateLabel_->setStyleSheet("font-size: 20px; font-weight: bold; color: #1f3a5f;");
    layout->addWidget(dateLabel_);

    // --- "What did you eat?" ---
    auto* askBox = new QGroupBox("What did you eat?");
    auto* askLayout = new QVBoxLayout(askBox);
    auto* askRow = new QHBoxLayout;
    sentence_ = new QLineEdit;
    sentence_->setObjectName("sentence");
    sentence_->setPlaceholderText("Example: 2 oz cooked rice and a 4 oz apple");
    mealAI_ = new QComboBox;
    mealAI_->addItems({"Breakfast", "Lunch", "Dinner", "Snack"});
    mealAI_->setCurrentText(mealForNow());
    analyzeBtn_ = new QPushButton("Calculate");
    askRow->addWidget(sentence_, 1);
    askRow->addWidget(mealAI_);
    askRow->addWidget(analyzeBtn_);
    status_ = new QLabel("Say if food is cooked or raw, and mention package labels (like calories on a milk carton).");
    status_->setWordWrap(true);
    status_->setStyleSheet("color: #5a6678;");
    askLayout->addLayout(askRow);
    askLayout->addWidget(status_);
    layout->addWidget(askBox);
    connect(analyzeBtn_, &QPushButton::clicked, this, &MainWindow::analyzeFood);
    connect(sentence_, &QLineEdit::returnPressed, this, &MainWindow::analyzeFood);

    // --- Progress ---
    auto* progBox = new QGroupBox("Today's progress");
    auto* progGrid = new QVBoxLayout(progBox);
    auto addBarRow = [&](const QString& label, const char* id, QProgressBar*& bar, QLabel*& text) {
        auto* row = new QHBoxLayout;
        auto* l = new QLabel(label);
        l->setMinimumWidth(70);
        bar = new QProgressBar;
        bar->setObjectName(id);
        bar->setRange(0, 100);
        bar->setTextVisible(false);
        text = new QLabel;
        text->setMinimumWidth(230);
        row->addWidget(l);
        row->addWidget(bar, 1);
        row->addWidget(text);
        progGrid->addLayout(row);
    };
    addBarRow("Calories", "cal", calBar_, calText_);
    addBarRow("Protein", "pro", proBar_, proText_);
    addBarRow("Fiber", "fib", fibBar_, fibText_);
    sugarFat_ = new QLabel;
    sugarFat_->setStyleSheet("font-size: 14px; color: #1f3a5f; padding-top: 4px;");
    progGrid->addWidget(sugarFat_);
    layout->addWidget(progBox);

    // --- Goals ---
    auto* goalBox = new QGroupBox("Daily goals (change them any time)");
    auto* goalRow = new QHBoxLayout(goalBox);
    calGoal_ = new QSpinBox;   calGoal_->setRange(1, 10000);   calGoal_->setSuffix(" kcal");
    proGoal_ = new QDoubleSpinBox; proGoal_->setRange(1, 1000); proGoal_->setDecimals(1); proGoal_->setSuffix(" g");
    fibGoal_ = new QDoubleSpinBox; fibGoal_->setRange(1, 1000); fibGoal_->setDecimals(1); fibGoal_->setSuffix(" g");
    goalRow->addWidget(new QLabel("Calories:")); goalRow->addWidget(calGoal_);
    goalRow->addWidget(new QLabel("Protein:"));  goalRow->addWidget(proGoal_);
    goalRow->addWidget(new QLabel("Fiber:"));    goalRow->addWidget(fibGoal_);
    layout->addWidget(goalBox);
    connect(calGoal_, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::goalsChanged);
    connect(proGoal_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::goalsChanged);
    connect(fibGoal_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::goalsChanged);

    // --- Food table ---
    auto* tableBox = new QGroupBox("Today's food log");
    auto* tableLayout = new QVBoxLayout(tableBox);
    foodTable_ = new QTableWidget(0, 7);
    foodTable_->setHorizontalHeaderLabels({"Meal", "Food", "kcal", "Protein (g)", "Fiber (g)", "Sugar (g)", "Sat fat (g)"});
    foodTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    foodTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    foodTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    foodTable_->verticalHeader()->setVisible(false);
    auto* delBtn = new QPushButton("Delete selected");
    tableLayout->addWidget(foodTable_);
    tableLayout->addWidget(delBtn, 0, Qt::AlignRight);
    layout->addWidget(tableBox, 1);
    connect(delBtn, &QPushButton::clicked, this, &MainWindow::deleteSelected);

    return page;
}

QWidget* MainWindow::buildManualTab() {
    auto* page = new QWidget;
    page->setObjectName("page");
    auto* layout = new QVBoxLayout(page);

    auto* box = new QGroupBox("Add a food by typing the numbers yourself (works without internet)");
    auto* form = new QFormLayout(box);
    name_ = new QLineEdit;
    name_->setPlaceholderText("Food name");
    meal_ = new QComboBox;
    meal_->addItems({"Breakfast", "Lunch", "Dinner", "Snack"});
    meal_->setCurrentText(mealForNow());
    cal_ = new QSpinBox;  cal_->setRange(0, 10000); cal_->setSuffix(" kcal");
    auto makeG = [](QDoubleSpinBox*& sb) {
        sb = new QDoubleSpinBox; sb->setRange(0, 1000); sb->setDecimals(1); sb->setSuffix(" g");
    };
    makeG(pro_); makeG(fib_); makeG(sugar_); makeG(sat_);
    auto* addBtn = new QPushButton("Add");
    form->addRow("Food:", name_);
    form->addRow("Meal:", meal_);
    form->addRow("Calories:", cal_);
    form->addRow("Protein:", pro_);
    form->addRow("Fiber:", fib_);
    form->addRow("Sugar:", sugar_);
    form->addRow("Saturated fat:", sat_);
    form->addRow("", addBtn);
    layout->addWidget(box);
    layout->addStretch(1);
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::addManual);
    return page;
}

QWidget* MainWindow::buildHistoryTab() {
    auto* page = new QWidget;
    page->setObjectName("page");
    auto* layout = new QVBoxLayout(page);
    historyTable_ = new QTableWidget(0, 6);
    historyTable_->setHorizontalHeaderLabels({"Date", "kcal", "Protein (g)", "Fiber (g)", "Sugar (g)", "Sat fat (g)"});
    historyTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    historyTable_->verticalHeader()->setVisible(false);
    layout->addWidget(historyTable_);
    return page;
}

void MainWindow::setBar(QProgressBar* bar, QLabel* label, double value, double goal,
                        const QString& unit, int digits) {
    double pct = goal > 0 ? value / goal * 100.0 : 0.0;
    bar->setValue(static_cast<int>(std::min(pct, 100.0)));
    label->setText(QString("%1 / %2 %3   (%4%)")
                       .arg(value, 0, 'f', digits)
                       .arg(goal, 0, 'f', digits)
                       .arg(unit)
                       .arg(pct, 0, 'f', 1));
}

void MainWindow::refresh() {
    const QString today = todayDate();
    dateLabel_->setText(QDate::currentDate().toString("dddd, MMMM d, yyyy"));

    updatingGoals_ = true;
    calGoal_->setValue(tracker_->goals().calories);
    proGoal_->setValue(tracker_->goals().protein);
    fibGoal_->setValue(tracker_->goals().fiber);
    updatingGoals_ = false;

    Totals t = tracker_->totalsFor(today.toStdString());
    const Goals& g = tracker_->goals();
    setBar(calBar_, calText_, t.calories, g.calories, "kcal", 0);
    setBar(proBar_, proText_, t.protein, g.protein, "g", 1);
    setBar(fibBar_, fibText_, t.fiber, g.fiber, "g", 1);
    sugarFat_->setText(QString("Sugar today: <b>%1 g</b> &nbsp;&nbsp;|&nbsp;&nbsp; Saturated fat today: <b>%2 g</b>")
                           .arg(one(t.sugar)).arg(one(t.satFat)));

    auto entries = tracker_->entriesFor(today.toStdString());
    foodTable_->setRowCount(static_cast<int>(entries.size()));
    for (int r = 0; r < static_cast<int>(entries.size()); ++r) {
        const FoodEntry& e = entries[r];
        auto* meal = cell(QString::fromStdString(e.getMeal()));
        meal->setData(Qt::UserRole, e.getId());
        foodTable_->setItem(r, 0, meal);
        foodTable_->setItem(r, 1, cell(QString::fromStdString(e.getName())));
        foodTable_->setItem(r, 2, cell(QString::number(e.getCalories()), true));
        foodTable_->setItem(r, 3, cell(one(e.getProtein()), true));
        foodTable_->setItem(r, 4, cell(one(e.getFiber()), true));
        foodTable_->setItem(r, 5, cell(one(e.getSugar()), true));
        foodTable_->setItem(r, 6, cell(one(e.getSatFat()), true));
    }

    auto days = tracker_->allDailyTotals();
    historyTable_->setRowCount(static_cast<int>(days.size()));
    int row = 0;
    for (auto it = days.rbegin(); it != days.rend(); ++it, ++row) {
        historyTable_->setItem(row, 0, cell(QString::fromStdString(it->first)));
        historyTable_->setItem(row, 1, cell(QString::number(it->second.calories), true));
        historyTable_->setItem(row, 2, cell(one(it->second.protein), true));
        historyTable_->setItem(row, 3, cell(one(it->second.fiber), true));
        historyTable_->setItem(row, 4, cell(one(it->second.sugar), true));
        historyTable_->setItem(row, 5, cell(one(it->second.satFat), true));
    }
}

void MainWindow::analyzeFood() {
    if (!analyzeBtn_->isEnabled()) return;   // already waiting for an answer
    QString text = sentence_->text().trimmed();
    if (text.isEmpty()) {
        QMessageBox::information(this, "What did you eat?",
                                 "Type what you ate first, for example: 2 oz cooked rice and a 4 oz apple");
        return;
    }
    if (apiKey().isEmpty() && !setApiKey()) return;

    pendingMeal_ = mealAI_->currentText();
    analyzeBtn_->setEnabled(false);
    status_->setStyleSheet("color: #1f3a5f;");
    status_->setText("Asking Claude...");
    client_->analyze(apiKey(), text);
}

void MainWindow::onAnalysis(const AnalysisResult& result) {
    analyzeBtn_->setEnabled(true);

    if (!result.ok) {
        status_->setStyleSheet("color: #b3261e;");
        status_->setText(result.error);
        return;
    }
    status_->setStyleSheet("color: #5a6678;");
    status_->setText("Check the numbers below, then add them to your log.");

    QStringList lines;
    double kcal = 0;
    for (const ParsedFood& f : result.items) {
        lines << describe(f);
        kcal += f.calories;
    }
    if (result.items.size() > 1) lines << QString("\nTotal: %1 kcal").arg(qRound(kcal));

    QMessageBox box(this);
    box.setWindowTitle("Add to " + pendingMeal_ + "?");
    box.setIcon(QMessageBox::Question);
    box.setText("Claude worked out:");
    box.setInformativeText(lines.join("\n\n") +
                           "\n\nThese are estimates. Add them to " + pendingMeal_ + "?");
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::Yes);
    if (box.exec() == QMessageBox::Yes) {
        addFoods(result.items, pendingMeal_);
        sentence_->clear();
        status_->setText("Added.");
    } else {
        status_->setText("Not added. Change your sentence and try again if you like.");
    }
}

void MainWindow::addFoods(const QVector<ParsedFood>& items, const QString& meal) {
    for (const ParsedFood& f : items) {
        tracker_->addEntry(todayDate().toStdString(), meal.toStdString(), f.name.toStdString(),
                           qRound(f.calories), f.protein, f.fiber, f.sugar, f.satFat);
    }
    tracker_->save();
    refresh();
}

void MainWindow::addManual() {
    QString name = name_->text().trimmed();
    if (!validName(name)) {
        QMessageBox::warning(this, "Invalid food name",
                             "Please enter a food name with at least one letter (no | character).");
        return;
    }
    ParsedFood f;
    f.name = name;
    f.calories = cal_->value();
    f.protein = pro_->value();
    f.fiber = fib_->value();
    f.sugar = sugar_->value();
    f.satFat = sat_->value();
    addFoods({f}, meal_->currentText());

    name_->clear();
    cal_->setValue(0); pro_->setValue(0); fib_->setValue(0); sugar_->setValue(0); sat_->setValue(0);
    name_->setFocus();
}

void MainWindow::deleteSelected() {
    int row = foodTable_->currentRow();
    if (row < 0 || !foodTable_->item(row, 0)) {
        QMessageBox::information(this, "Delete", "Click a food in the list first.");
        return;
    }
    int id = foodTable_->item(row, 0)->data(Qt::UserRole).toInt();
    tracker_->deleteEntry(id);
    tracker_->save();
    refresh();
}

void MainWindow::goalsChanged() {
    if (updatingGoals_) return;
    tracker_->goals().calories = calGoal_->value();
    tracker_->goals().protein = proGoal_->value();
    tracker_->goals().fiber = fibGoal_->value();
    tracker_->save();
    refresh();
}
