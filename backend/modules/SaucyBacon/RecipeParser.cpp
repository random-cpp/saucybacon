/**
 * This file is part of SaucyBacon.
 *
 * Copyright 2013-2015 (C) Giulio Collura <random.cpp@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include "RecipeParser.h"
#include "ApiKeys.h"

#include <QtCore>
#include <QtScript/QScriptEngine>

// static functions --------------------------------------------

static QJsonValue evaluate(const QString &expr) {
    QScriptEngine myEngine;
    auto result = myEngine.evaluate(expr);
    return result.isNumber() ? result.toNumber() : 0;
}

static QJsonArray parseIngredients(const QJsonArray &ingredients) {
    QJsonArray result;
    QRegularExpression regex("(?<quantity>[\\d\\-/?]+)[\\s+]?(?<unit>\\w+)?[\\s+](?<name>.*)",
                             QRegularExpression::CaseInsensitiveOption);
    for (int i = 0; i < ingredients.count(); i++) {
        QJsonObject ingredient;
        auto match = regex.match(ingredients[i].toString(), 0, QRegularExpression::PartialPreferCompleteMatch);
        if (match.hasMatch() || match.hasPartialMatch()) {
            ingredient["name"] = match.captured("name");
            ingredient["quantity"] = match.captured("quantity");
            ingredient["unit"] = match.captured("unit");
        } else {
            ingredient["name"] = ingredients[i].toString().trimmed();
            ingredient["quantity"] = 0;
            ingredient["unit"] = QString();
        }
        result.push_back(ingredient);
    }
    return result;
}

// -------------------------------------------------------------

RecipeParser::RecipeParser(QObject *parent) :
    QObject(parent) {

    RecipeRegex recipeRegex;
    // Default regex -- "a^" doesn't match anything
    recipeRegex["directions"] = QRegularExpression("a^");
    recipeRegex["preptime"] = QRegularExpression("a^");
    recipeRegex["cooktime"] = QRegularExpression("a^");
    m_services["default"] = recipeRegex;

    // Allrecipes
    recipeRegex["directions"] = QRegularExpression("<li><span class=\"plaincharacterwrap break\">(.*)</span></li>");
    recipeRegex["preptime"] = QRegularExpression("<span id=\"prepMinsSpan\"><em>(\\d+)</em>");
    recipeRegex["cooktime"] = QRegularExpression("<span id=\"cookMinsSpan\"><em>(\\d+)</em>");
    m_services["http://allrecipes.com"] = recipeRegex;

    // SimplyRecipes
    recipeRegex["directions"] = QRegularExpression("itemprop=\"recipeInstructions\">(.+?)</div>",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("class=\"preptime\".*?>(\\d+) ([mM]inutes|[hH]our[s]?)<span");
    recipeRegex["cooktime"] = QRegularExpression("class=\"cooktime\".*?>(\\d+) ([mM]inutes|[hH]our[s]?)<span");
    m_services["http://simplyrecipes.com"] = recipeRegex;

    // PioneerWoman
    recipeRegex["directions"] = QRegularExpression("itemprop=\"instructions\">(.+?)</div>",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("itemprop=\'prepTime\'.+?>(\\d+) ([mM]inutes|[hH]our[s]?)</time");
    recipeRegex["cooktime"] = QRegularExpression("itemprop=\'cookTime\'.+?>(\\d+) ([mM]inutes|[hH]our[s]?)</time");
    m_services["http://thepioneerwoman.com"] = recipeRegex;

    // Two peas and their pot
    recipeRegex["directions"] = QRegularExpression("<div class=\"instructions\">(.*?)</div>",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("class=\"preptime\".*?>(\\d+) ([mM]inutes|[hH]our[s]?)</span");
    recipeRegex["cooktime"] = QRegularExpression("class=\"cooktime\".*?>(\\d+) ([mM]inutes|[hH]our[s]?)</span");
    m_services["http://www.twopeasandtheirpod.com"] = recipeRegex;

    // Tasty Kitchen
    recipeRegex["directions"] = QRegularExpression("<span itemprop=\"instructions\">(.*?)</span>",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("itemprop=\'prepTime\'.*?>(\\d+) ([mM]inutes|[hH]our[s]?)</time");
    recipeRegex["cooktime"] = QRegularExpression("itemprop=\'cookTime\'.*?>(\\d+) ([mM]inutes|[hH]our[s]?)</time");
    m_services["http://tastykitchen.com"] = recipeRegex;

    // Jamie Oliver's Recipes
    recipeRegex["directions"] = QRegularExpression("<p class=\"instructions\">(.*?)</p>",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("class=\"preptime\".*?>(\\d+) ([mM]inutes|[hH]our[s]?)</span");
    recipeRegex["cooktime"] = QRegularExpression("class=\"cooktime\".*?>(\\d+) ([mM]inutes|[hH]our[s]?)</span");
    m_services["http://www.jamieoliver.com"] = recipeRegex;

    // Closet cooking
    recipeRegex["directions"] = QRegularExpression("<ol class=\"instructions\">(.*?)</ol>",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("class=\"prepTime\".*?>(\\d+) ([mM]inutes|[hH]our[s]?)</span");
    recipeRegex["cooktime"] = QRegularExpression("class=\"cookTime\".*?>(\\d+) ([mM]inutes|[hH]our[s]?)</span");
    m_services["http://closetcooking.com"] = recipeRegex;

    // 101 Cookbooks
    recipeRegex["directions"] = QRegularExpression("</blockquote>(.*?)<div class=\"recipetimes\">",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("class=\"preptime\".*?>(\\d+) ([mM]in|[hH]ours)");
    recipeRegex["cooktime"] = QRegularExpression("class=\"cooktime\".*?>(\\d+) ([mM]in|[hH]ours)");
    m_services["http://www.101cookbooks.com"] = recipeRegex;

    // Epicurious
    recipeRegex["directions"] = QRegularExpression("<div id=\"preparation\".+?>(.+?)</div>",
                                                   QRegularExpression::DotMatchesEverythingOption |
                                                   QRegularExpression::CaseInsensitiveOption);
    recipeRegex["preptime"] = QRegularExpression("class=\"prepTime\">.+?(\\d+) (hour[s]?|min[a-z+]?).+?<span",
                                                 QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["cooktime"] = QRegularExpression("a^");
    m_services["http://www.epicurious.com"] = recipeRegex;

    // BBC good food
    recipeRegex["directions"] = QRegularExpression("section id=\"recipe-method\".+?>(.+?)</section",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("class=\"cooking-time-prep\".+?>.+?(\\d+) (hour[s]?|min[a-z+]+).+?</span",
                                                 QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["cooktime"] = QRegularExpression("class=\"cooking-time-cook\".+?>.+?(\\d+) (hour[s]?|min[a-z+]+).+?</span",
                                                 QRegularExpression::DotMatchesEverythingOption);
    m_services["http://www.bbcgoodfood.com"] = recipeRegex;

    // BBC.co.uk Food
    recipeRegex["directions"] = QRegularExpression("div id=\"preparation\".+?>(.+?)</div",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("class=\"prepTime\".+?>.+?(\\d+) (hour[s]?|min[a-z+]+).?</span",
                                                 QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["cooktime"] = QRegularExpression("class=\"cookTime\".+?>.+?(\\d+) (hour[s]?|min[a-z+]+).?</span",
                                                 QRegularExpression::DotMatchesEverythingOption);
    m_services["http://www.bbc.co.uk/food"] = recipeRegex;

    // BonAppetit
    recipeRegex["directions"] = QRegularExpression("class=\"prep-steps\".+?>(.+?)<div class=\"recipe-footer",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("itemprop=\"prepTime\".+?>.+?(\\d+) (hour[s]?|min[a-z+]+).?</span",
                                                 QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["cooktime"] = QRegularExpression("itemprop=\"totalTime\".+?>.+?(\\d+) (hour[s]?|min[a-z+]+).?</span",
                                                 QRegularExpression::DotMatchesEverythingOption);
    m_services["http://www.bonappetit.com"] = recipeRegex;

    // Cookstr
    recipeRegex["directions"] = QRegularExpression("itemprop=\"recipeInstructions\".+?>(.+?)</div",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("a^");
    recipeRegex["cooktime"] = QRegularExpression("a^");
    m_services["http://www.cookstr.com"] = recipeRegex;

    // Chow
    recipeRegex["directions"] = QRegularExpression("itemprop=\"instructions\".+?>(.+?)</div",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("a^");
    recipeRegex["cooktime"] = QRegularExpression("a^");
    m_services["http://www.chow.com"] = recipeRegex;

    // Smitten Kitchen
    recipeRegex["directions"] = QRegularExpression("class=\"entry\"+?>(.+?)<script",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("a^");
    recipeRegex["cooktime"] = QRegularExpression("a^");
    m_services["http://www.smittenkitchen.com"] = recipeRegex;

    // Whats Gaby cooking
    recipeRegex["directions"] = QRegularExpression("class=\"instructions\"+?>(.+?)</ol",
                                                   QRegularExpression::DotMatchesEverythingOption);
    recipeRegex["preptime"] = QRegularExpression("a^");
    recipeRegex["cooktime"] = QRegularExpression("a^");
    m_services["http://whatsgabycooking.com"] = recipeRegex;

    // And more soon...

    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::finished, this, &RecipeParser::replyFinished);

    QDir dataPath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    if (!dataPath.mkdir("imgs") && !dataPath.cd("imgs"))
        qWarning() << "Failed to make data folder" << dataPath.absolutePath();

    m_destPath = dataPath.absolutePath();

    loading(false);
}

RecipeParser::~RecipeParser() {

}

void RecipeParser::get(const QString &recipeId, const QString &recipeUrl, const QString &serviceUrl, const QString &imageUrl) {
    loading(true);
    m_parseHtml = false;
    m_parseJson = false;
    m_parseImage = false;

    QUrl ingredientsUrl(ApiKeys::F2FGETURL);
    QUrl directionsUrl(recipeUrl);
    m_service = serviceUrl;
    QUrl photoUrl(imageUrl);

    QUrlQuery query;
    query.addQueryItem("key", ApiKeys::F2FKEY);
    query.addQueryItem("rId", recipeId);
    ingredientsUrl.setQuery(query);
    QNetworkRequest ingredientsReq(ingredientsUrl);
    m_manager->get(ingredientsReq);

    QNetworkRequest directionReq(directionsUrl);
    directionReq.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)");
    m_manager->get(directionReq);

    QNetworkRequest photoReq(photoUrl);
    m_photoName = imageUrl.split("/").back();
    m_manager->get(photoReq);
}

void RecipeParser::replyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {

        bool apiResponse = QUrl(ApiKeys::F2FURL).isParentOf(reply->url());
        if (apiResponse) {
            parseJson(reply->readAll());
        } else if (reply->url().toString().contains(m_photoName)) {
            parseImage(reply->readAll());
        } else {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode == 301) {
                QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                m_manager->get(QNetworkRequest(redirectUrl));
            }
            parseHtml(reply->readAll());
        }

    } else {
        qDebug() << reply->errorString();
    }
    reply->deleteLater();
}

void RecipeParser::parseHtml(const QByteArray &html) {
    RecipeRegex defaultRegex;
    bool supported;
    QString directions;
    int preptime = 0, cooktime = 0, offset = 1;

    if (m_services.contains(m_service)) {
        defaultRegex = m_services[m_service];
        supported = true;
    } else {
        supported = false;
        qDebug() << "Site not supported yet: " + m_service;
    }

    if (supported) {
        // TODO: Remove QRegularExpressionMatchIterator and use QRegExpMatch only.
        QRegularExpressionMatchIterator matchDirections = defaultRegex["directions"].globalMatch(html);
        while (matchDirections.hasNext()) {
            auto match = matchDirections.next();
            directions.append(match.captured(1).replace(QRegularExpression("<.+?>"), "").simplified());
            directions.append("<br />");
        }

        auto match = defaultRegex["preptime"].match(html);
        if (match.hasMatch()) {
            if (match.captured(2).contains(QRegularExpression("[hH]our[s]?")))
                offset = 60;
            preptime = match.captured(1).toInt() * offset;
        } else
            preptime = 0;

        offset = 1;
        match = defaultRegex["cooktime"].match(html);
        if (match.hasMatch()) {
            if (match.captured(2).contains(QRegularExpression("[hH]our[s]?")))
                offset = 60;
            cooktime = match.captured(1).toInt() * offset;
        } else
            cooktime = 0;

    } else {
        directions.append(tr("This website is supported yet. It was impossible to load the directions."));
    }

    // Default copyright string
    directions.append(tr("<br />Recipe from %1.<br />Directions are not part of F2F API.").arg(m_service));

    // Check if we get acceptable values or some junk
    if (preptime > 200000 || preptime < 0)
        preptime = 0;
    if (cooktime > 200000 || cooktime < 0)
        cooktime = 0;

    m_contents["directions"] = directions;
    m_contents["preptime"] = preptime;
    m_contents["cooktime"] = cooktime;

    m_parseHtml = true;
    hasFinishedParsing();
}

void RecipeParser::parseJson(const QByteArray &json) {
    QJsonObject recipe = QJsonDocument::fromJson(json).object()["recipe"].toObject();
    m_contents["name"] = recipe["title"].toString();
    m_contents["ingredients"] = parseIngredients(recipe["ingredients"].toArray());
    m_contents["source"] = recipe["source_url"].toString();
    m_contents["f2f"] = recipe["f2f_url"].toString();

    m_contents["categories"] = QJsonArray();
    m_contents["restriction"] = 0;
    m_contents["favorite"] = 0;
    m_contents["saved"] = false;

    m_parseJson = true;
    hasFinishedParsing();
}

void RecipeParser::parseImage(const QByteArray &imgData) {
    QFile photo;
    photo.setFileName(m_destPath + "/" + m_photoName);
    photo.open(QIODevice::WriteOnly);
    photo.write(imgData);
    photo.close();

    m_contents["photos"] = QJsonArray::fromStringList(QStringList(photo.fileName()));

    m_parseImage = true;
    hasFinishedParsing();
}

void RecipeParser::hasFinishedParsing() {
    // If all processes are completed, trigger all signals
    if (m_parseJson && m_parseHtml && m_parseImage) {
        emit loading(false);
        emit recipeAvailable(m_contents);
    }
}
