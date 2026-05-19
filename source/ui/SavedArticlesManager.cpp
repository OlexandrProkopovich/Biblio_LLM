#include "ui/SavedArticlesManager.h"
#include <QJsonObject>
#include <QJsonDocument>

QString SavedArticlesManager::dirPath() const 
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/saved";
    QDir().mkpath(path);

    return path;
}

void SavedArticlesManager::save(const ArticleData& a) 
{
    QJsonObject obj;
    obj["id"] = a.id;
    obj["title"] = a.title;
    obj["authors"] = a.authors;
    obj["abstract"] = a.abstract;
    obj["url"] = a.url;
    obj["published"] = a.published;
    obj["publishedRaw"] = a.publishedRaw;
    obj["doi"] = a.doi;
    obj["journal"] = a.journal;

    QFile file(dirPath() + "/" + a.id + ".json");
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(obj).toJson());
}

void SavedArticlesManager::remove(const QString& id) 
{
    QFile::remove(dirPath() + "/" + id + ".json");
}

QList<ArticleData> SavedArticlesManager::loadAll() const 
{
    QList<ArticleData> result;
    QDir dir(dirPath());

    for (const QString& name : dir.entryList({ "*.json" }, QDir::Files)) 
    {
        QFile file(dir.filePath(name));
        file.open(QIODevice::ReadOnly);
        QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();

        ArticleData a;
        a.id = obj["id"].toString();
        a.title = obj["title"].toString();
        a.authors = obj["authors"].toString();
        a.abstract = obj["abstract"].toString();
        a.url = obj["url"].toString();
        a.published = obj["published"].toString();
        a.publishedRaw = obj["publishedRaw"].toString();
        a.doi = obj["doi"].toString();
        a.journal = obj["journal"].toString();

        result.append(a);
    }
    return result;
}