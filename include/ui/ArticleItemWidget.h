#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMetaType>

struct ArticleData
{
    QString id;
    QString title;
    QString authors;
    QString abstract;
    QString url;
    QString published;
    QString publishedRaw;
    QString journal;
    QString doi;
};

Q_DECLARE_METATYPE(ArticleData)

enum class ExportFormat 
{ 
    PlainText, 
    BibTeX, 
    RIS 
};

enum WidgetMode 
{ 
    SearchMode, 
    SavedMode 
};

class ArticleItemWidget : public QWidget 
{
    Q_OBJECT
public:
    explicit ArticleItemWidget(const ArticleData& data, WidgetMode mode = SearchMode, QWidget* parent = nullptr);

signals:
    void saveRequested(ArticleData data);
    void exportRequested(ArticleData data, ExportFormat format);
    void deleteRequested(ArticleData data);

private:
    ArticleData m_data;
    QLabel* m_titleLabel;
    QLabel* m_authorsLabel;
    QLabel* m_abstractLabel;
};