#ifndef CLAUDECLIENT_H
#define CLAUDECLIENT_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVector>

class QNetworkAccessManager;

// One food item worked out by Claude (numbers are for the amount eaten).
struct ParsedFood {
    QString name;
    double calories = 0;
    double protein = 0;
    double fiber = 0;
    double sugar = 0;
    double satFat = 0;
    QString assumption;   // what Claude had to guess, if anything
};

struct AnalysisResult {
    bool ok = false;
    QString error;                 // plain-language message when ok is false
    QVector<ParsedFood> items;
};

// Sends a sentence like "2 oz cooked rice and a 4 oz apple" to the Claude API
// and turns the answer into numbers.
class ClaudeClient : public QObject {
    Q_OBJECT
public:
    explicit ClaudeClient(QObject* parent = nullptr);

    void analyze(const QString& apiKey, const QString& description);

    // These two are separate so they can be tested without internet.
    static QByteArray buildRequestBody(const QString& description);
    static AnalysisResult parseReply(int httpStatus, const QByteArray& body);

signals:
    void finished(const AnalysisResult& result);

private:
    QNetworkAccessManager* net_;
};

#endif
