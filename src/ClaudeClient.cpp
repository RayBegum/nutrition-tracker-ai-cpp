#include "ClaudeClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <algorithm>

namespace {

// Change this one line to use a different Claude model.
const char* kModel = "claude-sonnet-5-5";
const char* kApiUrl = "https://api.anthropic.com/v1/messages";

const char* kSystemPrompt =
    "You are the nutrition calculator inside a food-tracking app. The user describes what they "
    "ate in everyday words. Work out the nutrition for the amount they actually ate.\n"
    "Rules:\n"
    "- Respect cooked vs raw vs dry. Cooked rice and raw rice have very different numbers per ounce.\n"
    "- If the user gives numbers from a package label (calories, protein, serving size), use those "
    "numbers and scale them to the amount eaten instead of guessing.\n"
    "- Convert units yourself (1 oz = 28.35 g, 1 cup, ml, pieces, and so on).\n"
    "- If something important is missing (cooked or raw, size, brand), make the most typical "
    "assumption and say so in \"assumption\". Leave \"assumption\" empty when nothing was guessed.\n"
    "- Give one item per food or drink. Use realistic, careful estimates; do not exaggerate precision.\n"
    "- Reply with ONLY a JSON object, no other text, in exactly this shape:\n"
    "{\"items\":[{\"name\":\"short food name with the amount, e.g. Cooked white rice (2 oz)\","
    "\"calories\":0,\"protein_g\":0,\"fiber_g\":0,\"sugar_g\":0,\"saturated_fat_g\":0,"
    "\"assumption\":\"\"}],\"error\":\"\"}\n"
    "- All numbers are plain numbers for the amount eaten (not per 100 g).\n"
    "- If the message does not describe any food or drink, return {\"items\":[],\"error\":\"short "
    "friendly explanation\"}.";

double number(const QJsonObject& o, const char* key) {
    double v = o.value(key).toDouble(0.0);
    return std::max(0.0, v);   // never allow negative values
}

QString apiErrorMessage(int status, const QByteArray& body) {
    QString detail;
    QJsonObject err = QJsonDocument::fromJson(body).object().value("error").toObject();
    if (!err.isEmpty()) detail = err.value("message").toString();

    switch (status) {
        case 401:
            return "The API key was not accepted. Check it under Settings > Set API key.";
        case 403:
            return "This API key is not allowed to do that. " + detail;
        case 429:
            return "Too many requests right now. Wait a moment and try again.";
        case 400:
            return "The request was rejected: " + detail;
        case 529:
        case 500:
        case 502:
        case 503:
            return "Claude is busy right now. Please try again in a moment.";
        default:
            return QString("Something went wrong (code %1). %2").arg(status).arg(detail);
    }
}

}  // namespace

ClaudeClient::ClaudeClient(QObject* parent)
    : QObject(parent), net_(new QNetworkAccessManager(this)) {}

QByteArray ClaudeClient::buildRequestBody(const QString& description) {
    QJsonObject message;
    message["role"] = "user";
    message["content"] = description;

    QJsonObject body;
    body["model"] = kModel;
    body["max_tokens"] = 1024;
    body["system"] = kSystemPrompt;
    body["messages"] = QJsonArray{message};
    return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

AnalysisResult ClaudeClient::parseReply(int httpStatus, const QByteArray& body) {
    AnalysisResult result;
    if (httpStatus != 200) {
        result.error = apiErrorMessage(httpStatus, body);
        return result;
    }

    // The reply is {"content":[{"type":"text","text":"..."}], ...}
    QString text;
    for (const QJsonValue& block : QJsonDocument::fromJson(body).object().value("content").toArray()) {
        QJsonObject b = block.toObject();
        if (b.value("type").toString() == "text") text += b.value("text").toString();
    }

    // Claude should answer with only JSON, but cut anything around the braces to be safe.
    int start = text.indexOf('{');
    int end = text.lastIndexOf('}');
    if (start < 0 || end <= start) {
        result.error = "Claude's answer could not be read. Please try again.";
        return result;
    }
    QJsonParseError parseError;
    QJsonObject root = QJsonDocument::fromJson(text.mid(start, end - start + 1).toUtf8(), &parseError).object();
    if (parseError.error != QJsonParseError::NoError) {
        result.error = "Claude's answer could not be read. Please try again.";
        return result;
    }

    for (const QJsonValue& v : root.value("items").toArray()) {
        QJsonObject o = v.toObject();
        ParsedFood f;
        f.name = o.value("name").toString().trimmed();
        f.name.remove('|');                       // | is used as a separator in the save file
        if (f.name.isEmpty()) continue;
        f.calories = number(o, "calories");
        f.protein = number(o, "protein_g");
        f.fiber = number(o, "fiber_g");
        f.sugar = number(o, "sugar_g");
        f.satFat = number(o, "saturated_fat_g");
        f.assumption = o.value("assumption").toString().trimmed();
        result.items.append(f);
    }

    if (result.items.isEmpty()) {
        QString why = root.value("error").toString().trimmed();
        result.error = why.isEmpty()
            ? "I couldn't find any food in that. Try something like \"2 oz cooked rice and a 4 oz apple\"."
            : why;
        return result;
    }
    result.ok = true;
    return result;
}

void ClaudeClient::analyze(const QString& apiKey, const QString& description) {
    QNetworkRequest request{QUrl(kApiUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", apiKey.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");
    request.setTransferTimeout(45000);   // 45 seconds

    QNetworkReply* reply = net_->post(request, buildRequestBody(description));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray body = reply->readAll();
        QNetworkReply::NetworkError netError = reply->error();
        reply->deleteLater();

        if (status == 0) {   // no answer from the server at all
            AnalysisResult r;
            r.error = (netError == QNetworkReply::OperationCanceledError)
                ? "Claude took too long to answer. Please try again."
                : "Could not reach Claude. Check your internet connection.";
            emit finished(r);
            return;
        }
        emit finished(parseReply(status, body));
    });
}
