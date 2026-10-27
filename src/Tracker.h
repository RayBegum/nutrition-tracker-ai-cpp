#ifndef TRACKER_H
#define TRACKER_H

#include "FoodEntry.h"
#include <map>
#include <string>
#include <vector>

// Daily goals the user wants to reach.
struct Goals {
    int calories = 2000;
    double protein = 90.0;   // grams
    double fiber = 28.0;     // grams
};

// Totals for one day.
struct Totals {
    int calories = 0;
    double protein = 0.0;
    double fiber = 0.0;
    double sugar = 0.0;
    double satFat = 0.0;
};

// Holds goals and food entries, and handles saving/loading.
class Tracker {
public:
    Tracker(const std::string& foodFile, const std::string& goalFile);

    void load();
    void save() const;

    Goals& goals() { return goals_; }
    const Goals& goals() const { return goals_; }

    void addEntry(const std::string& date, const std::string& meal, const std::string& name,
                  int calories, double protein, double fiber, double sugar = 0.0, double satFat = 0.0);
    bool deleteEntry(int id);

    std::vector<FoodEntry> entriesFor(const std::string& date) const;
    Totals totalsFor(const std::string& date) const;
    std::map<std::string, Totals> allDailyTotals() const;   // sorted by date

private:
    static void add(Totals& t, const FoodEntry& e);

    std::string foodFile_, goalFile_;
    std::vector<FoodEntry> entries_;
    Goals goals_;
    int nextId_ = 1;
};

#endif
