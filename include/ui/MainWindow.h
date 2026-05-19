#pragma once
#include <QMainWindow>
#include <QCompleter>
#include <QStringListModel>

#include "biblio/CBibAnalyzer.h"
#include "biblio/CArxivSource.h"
#include "biblio/CPubMedSource.h"
#include "biblio/CSemanticScholarSource.h"

#include "ui/SavedArticlesManager.h"
#include "ui/ArticleItemWidget.h"

#include <QtConcurrent>
#include <QFutureWatcher>
#include <QListWidgetItem>

namespace Ui
{
    class MainWindow;
}

enum ArticleSource
{
    Arxiv,
    PubMed,
    SemanticScholar
};

enum RankingMode
{
    RankByModel,
    RankByTFIDF
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    void loadSavedArticles();
    void appendArticles(const std::vector<CArticle>& articles);
    void addArticleToList(const ArticleData& data);
    void renderArticles(const std::vector<ArticleData>& articles);
    void showDetail(const ArticleData& data, bool saved = false);
    void applySort();
    void applySavedSort();
    void addToHistory(const QString& query);
    std::unique_ptr<CArticleSource> createSource(ArticleSource source) const;
    ArticleData toArticleData(const CArticle& article) const;
    void triggerReRank();

private slots:
    void onNavItemClicked(int row);
    void onSearchClicked();
    void onSearchFinished();
    void onLoadMoreFinished();
    void onReRankFinished();
    void onExportRequested(ArticleData data, ExportFormat format);
    void onScrollChanged(int value);
    void onSortChanged(int index);
    void onRankingModeChanged(int index);
    void onArticleSelected(QListWidgetItem* current, QListWidgetItem* previous);
    void onSavedSortChanged(int index);
    void onSavedArticleSelected(QListWidgetItem* current, QListWidgetItem* previous);

private:
    Ui::MainWindow* ui;

    CBibAnalyzer* m_analyzer;
    SavedArticlesManager m_savedManager;

    QFutureWatcher<std::vector<CArticle>>* m_watcher;
    QFutureWatcher<std::vector<CArticle>>* m_loadMoreWatcher;
    QFutureWatcher<std::vector<CArticle>>* m_reRankWatcher;

    QCompleter* m_completer;
    QStringListModel* m_historyModel;
    QStringList m_searchHistory;

    std::vector<ArticleData> m_articles;
    std::vector<CArticle> m_rawArticles;

    QString m_currentQuery;
    int m_currentOffset = 0;
    ArticleSource m_currentSource = Arxiv;
    RankingMode m_rankingMode = RankByModel;
    bool m_isLoadingMore = false;
};