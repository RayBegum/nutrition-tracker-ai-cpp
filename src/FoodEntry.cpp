#include "FoodEntry.h"
#include <sstream>
#include <vector>

FoodEntry::FoodEntry(int id, const std::string& date, const std::string& meal,
                     const std::string& name, int calories, double protein, double fiber,
                     double sugar, double satFat)
    : id_(id), date_(date), meal_(meal), name_(name), calories_(calories),
      protein_(protein), fiber_(fiber), sugar_(sugar), satFat_(satFat) {}

// File format: id|date|meal|calories|protein|fiber|sugar|satFat|name
std::string FoodEntry::toFileLine() const {
    std::ostringstream out;
    out << id_ << '|' << date_ << '|' << meal_ << '|' << calories_ << '|' << protein_ << '|'
        << fiber_ << '|' << sugar_ << '|' << satFat_ << '|' << name_;
    return out.str();
}

// Also reads the older 7-field format (id|date|meal|calories|protein|fiber|name),
// so files saved by version 0 still load.
bool FoodEntry::fromFileLine(const std::string& line, FoodEntry& out) {
    std::vector<std::string> parts;
    std::istringstream in(line);
    std::string piece;
    while (std::getline(in, piece, '|')) parts.push_back(piece);
    if (parts.size() != 7 && parts.size() != 9) return false;

    try {
        bool isNew = parts.size() == 9;
        double sugar = isNew ? std::stod(parts[6]) : 0.0;
        double satFat = isNew ? std::stod(parts[7]) : 0.0;
        const std::string& name = parts.back();
        out = FoodEntry(std::stoi(parts[0]), parts[1], parts[2], name, std::stoi(parts[3]),
                        std::stod(parts[4]), std::stod(parts[5]), sugar, satFat);
    } catch (...) {
        return false;
    }
    return true;
}
