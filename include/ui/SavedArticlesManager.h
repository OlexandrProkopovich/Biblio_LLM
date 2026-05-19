#pragma once
#include "ui/ArticleItemWidget.h"
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

class SavedArticlesManager {
public:
    void save(const ArticleData& article);
    void remove(const QString& id);
    QList<ArticleData> loadAll() const;

private:
    QString dirPath() const;
};