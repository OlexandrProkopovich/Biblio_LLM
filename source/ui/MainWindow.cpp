#include "ui/MainWindow.h"
#include "ui_MainWindow.h"

#include "ui/ArticleItemWidget.h"
#include "ui/ExportDialog.h"

#include <QFileDialog>
#include <QDialog>
#include <QTextEdit>
#include <QClipboard>
#include <QCheckBox>
#include <QScrollBar>
#include <QDateTime>
#include <QApplication>

// 1. ������ ���������� �� ����� ������� ������
// 2. ������ ���� �� �����������


static QString formatDate(const QString& raw)
{
    if (raw.isEmpty()) return {};

    QDateTime dt = QDateTime::fromString(raw, Qt::ISODate);
    if (dt.isValid())
        return dt.toString("MMM d, yyyy");

    return raw;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->navList->setCurrentRow(0);

    connect(ui->navList, &QListWidget::currentRowChanged, this, &MainWindow::onNavItemClicked);

    connect(ui->searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    connect(ui->queryInput, &QLineEdit::returnPressed, this, &MainWindow::onSearchClicked);

    connect(ui->articlesList->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MainWindow::onScrollChanged);

    connect(ui->sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSortChanged);

    connect(ui->rankingModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onRankingModeChanged);

    connect(ui->articlesList, &QListWidget::currentItemChanged,
            this, &MainWindow::onArticleSelected);

    connect(ui->detailCloseBtn, &QPushButton::clicked, this, [this]() {
        ui->detailPanel->setVisible(false);
        ui->articlesList->clearSelection();
    });

    connect(ui->savedSortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSavedSortChanged);

    connect(ui->savedList, &QListWidget::currentItemChanged,
            this, &MainWindow::onSavedArticleSelected);

    connect(ui->savedDetailCloseBtn, &QPushButton::clicked, this, [this]() {
        ui->savedDetailPanel->setVisible(false);
        ui->savedList->clearSelection();
    });


    m_historyModel = new QStringListModel(this);
    m_completer = new QCompleter(m_historyModel, this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchContains);
    ui->queryInput->setCompleter(m_completer);

    m_analyzer = new CBibAnalyzer(nullptr, 64, 32, 128);
    m_analyzer->LoadModel("model.bin");

    m_watcher = new QFutureWatcher<std::vector<CArticle>>(this);
    m_loadMoreWatcher = new QFutureWatcher<std::vector<CArticle>>(this);
    m_reRankWatcher = new QFutureWatcher<std::vector<CArticle>>(this);

    connect(m_watcher, &QFutureWatcher<std::vector<CArticle>>::finished, this, &MainWindow::onSearchFinished);
    connect(m_loadMoreWatcher, &QFutureWatcher<std::vector<CArticle>>::finished, this, &MainWindow::onLoadMoreFinished);
    connect(m_reRankWatcher, &QFutureWatcher<std::vector<CArticle>>::finished, this, &MainWindow::onReRankFinished);

    ui->queryInput->setText("machine learning");
    onSearchClicked();
}

MainWindow::~MainWindow()
{
    delete m_analyzer;
    delete ui;
}

void MainWindow::loadSavedArticles()
{
    ui->savedList->clear();
    ui->savedDetailPanel->setVisible(false);

    auto saved = m_savedManager.loadAll();

    if (saved.empty())
    {
        auto* item = new QListWidgetItem("No saved articles yet.", ui->savedList);
        item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        return;
    }

    int sortIdx = ui->savedSortCombo->currentIndex();
    auto sortKey = [](const ArticleData& a) {
        return a.publishedRaw.isEmpty() ? a.published : a.publishedRaw;
    };
    if (sortIdx == 1)
        std::stable_sort(saved.begin(), saved.end(), [&](const ArticleData& a, const ArticleData& b) {
            return sortKey(a) > sortKey(b);
        });
    else if (sortIdx == 2)
        std::stable_sort(saved.begin(), saved.end(), [&](const ArticleData& a, const ArticleData& b) {
            return sortKey(a) < sortKey(b);
        });

    for (const ArticleData& data : saved)
    {
        auto* itemWidget = new ArticleItemWidget(data, SavedMode);
        connect(itemWidget, &ArticleItemWidget::deleteRequested, this, [this](ArticleData d)
        {
            m_savedManager.remove(d.id);
            loadSavedArticles();
        });
        connect(itemWidget, &ArticleItemWidget::exportRequested, this, &MainWindow::onExportRequested);

        auto* item = new QListWidgetItem(ui->savedList);
        item->setData(Qt::UserRole, QVariant::fromValue(data));
        itemWidget->adjustSize();
        item->setSizeHint(itemWidget->sizeHint());

        ui->savedList->setItemWidget(item, itemWidget);
    }
}

std::unique_ptr<CArticleSource> MainWindow::createSource(ArticleSource source) const
{
    switch (source)
    {
        case Arxiv:           return std::make_unique<CArxiveSource>();
        case PubMed:          return std::make_unique<CPubMedSource>();
        case SemanticScholar: return std::make_unique<CSemanticScholarSource>("s2k-Ckjk1mrPl3yZ1fQB9G6h8gA9jOXbHejacidOEP3O");
    }
    return std::make_unique<CArxiveSource>();
}

void MainWindow::addToHistory(const QString& query)
{
    m_searchHistory.removeAll(query);
    m_searchHistory.prepend(query);
    if (m_searchHistory.size() > 20)
        m_searchHistory.removeLast();
    m_historyModel->setStringList(m_searchHistory);
}

void MainWindow::onNavItemClicked(int row)
{
    ui->stackedWidget->setCurrentIndex(row);
    if (row == 1)
        loadSavedArticles();
}

void MainWindow::onSearchClicked()
{
    QString query = ui->queryInput->text().trimmed();
    if (query.isEmpty())
        return;

    m_currentQuery = query;
    m_currentOffset = 0;
    m_currentSource = static_cast<ArticleSource>(ui->sourceCombo->currentIndex());
    m_rankingMode = (ui->rankingModeCombo->currentIndex() == 0) ? RankByModel : RankByTFIDF;

    addToHistory(query);

    ui->searchButton->setEnabled(false);
    ui->progressBar->setVisible(true);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->articlesList->clear();
    m_articles.clear();
    m_rawArticles.clear();
    ui->detailPanel->setVisible(false);
    ArticleSource source = m_currentSource;
    std::string searchQuery = m_currentQuery.toStdString();
    auto future = QtConcurrent::run([this, source, searchQuery]() -> std::vector<CArticle>
    {
        auto src = createSource(source);
        return src->GetArticles(searchQuery, 0);
    });

    m_watcher->setFuture(future);
}

ArticleData MainWindow::toArticleData(const CArticle& article) const
{
    ArticleData data;
    data.id = QString::fromStdString(article.url).replace("http://", "").replace("https://", "").replace("/", "_").replace(".", "_");
    data.title = QString::fromStdString(article.title);
    data.authors = QString::fromStdString(article.authors.empty() ? "" : article.authors[0]);
    data.abstract = QString::fromStdString(article.abstract);
    data.url = QString::fromStdString(article.url);
    data.publishedRaw = QString::fromStdString(article.published);
    data.published = formatDate(data.publishedRaw);
    data.journal = QString::fromStdString(article.journal);
    data.doi = QString::fromStdString(article.doi);
    return data;
}

void MainWindow::appendArticles(const std::vector<CArticle>& results)
{
    auto* bar = ui->articlesList->verticalScrollBar();
    bar->blockSignals(true);
    for (const auto& article : results)
    {
        ArticleData data = toArticleData(article);
        m_articles.push_back(data);
        addArticleToList(data);
    }
    bar->blockSignals(false);
}

void MainWindow::addArticleToList(const ArticleData& data)
{
    auto* itemWidget = new ArticleItemWidget(data);

    connect(itemWidget, &ArticleItemWidget::saveRequested, this, [this](ArticleData d)
    {
        m_savedManager.save(d);
    });
    connect(itemWidget, &ArticleItemWidget::exportRequested, this, &MainWindow::onExportRequested);

    auto* item = new QListWidgetItem(ui->articlesList);
    item->setData(Qt::UserRole, QVariant::fromValue(data));
    itemWidget->adjustSize();
    item->setSizeHint(itemWidget->sizeHint());

    ui->articlesList->setItemWidget(item, itemWidget);
}

void MainWindow::renderArticles(const std::vector<ArticleData>& articles)
{
    auto* bar = ui->articlesList->verticalScrollBar();
    bar->blockSignals(true);
    ui->articlesList->clear();
    for (const ArticleData& data : articles)
        addArticleToList(data);
    bar->blockSignals(false);
}

void MainWindow::applySort()
{
    auto sorted = m_articles;
    int sortIndex = ui->sortCombo->currentIndex();

    if (sortIndex == 1)
        std::stable_sort(sorted.begin(), sorted.end(), [](const ArticleData& a, const ArticleData& b) {
            return a.publishedRaw > b.publishedRaw;
        });
    else if (sortIndex == 2)
        std::stable_sort(sorted.begin(), sorted.end(), [](const ArticleData& a, const ArticleData& b) {
            return a.publishedRaw < b.publishedRaw;
        });

    renderArticles(sorted);
}

void MainWindow::showDetail(const ArticleData& data, bool saved)
{
    QWidget*   panel     = saved ? ui->savedDetailPanel    : ui->detailPanel;
    QSplitter* splitter  = saved ? ui->savedContentSplitter: ui->contentSplitter;
    QLabel*    titleLbl  = saved ? ui->savedDetailTitle    : ui->detailTitle;
    QLabel*    metaLbl   = saved ? ui->savedDetailMeta     : ui->detailMeta;
    QTextEdit* abstractEd= saved ? ui->savedDetailAbstract : ui->detailAbstract;

    bool wasHidden = !panel->isVisible();
    panel->setVisible(true);
    if (wasHidden)
    {
        int total = splitter->width();
        splitter->setSizes({ total * 2 / 3, total / 3 });
    }

    QString titleHtml = data.url.isEmpty()
        ? "<b>" + data.title + "</b>"
        : "<b><a href=\"" + data.url + "\" style=\"color:#cba6f7; text-decoration:none;\">" + data.title + "</a></b>";
    titleLbl->setText(titleHtml);

    QStringList meta;
    if (!data.authors.isEmpty())   meta << data.authors;
    if (!data.published.isEmpty()) meta << data.published;
    if (!data.journal.isEmpty())   meta << data.journal;
    if (!data.doi.isEmpty())       meta << "DOI: " + data.doi;
    metaLbl->setText(meta.join(" | "));

    abstractEd->setPlainText(data.abstract);
}

void MainWindow::onSearchFinished()
{
    auto results = m_watcher->result();
    m_currentOffset = 50;
    m_rawArticles = results;

    if (m_rawArticles.empty())
    {
        ui->progressBar->setVisible(false);
        ui->searchButton->setEnabled(true);
        QApplication::restoreOverrideCursor();
        return;
    }


    triggerReRank();
}

void MainWindow::onLoadMoreFinished()
{
    m_isLoadingMore = false;

    auto newArticles = m_loadMoreWatcher->result();
    if (newArticles.empty())
    {
        ui->progressBar->setVisible(false);
        QApplication::restoreOverrideCursor();
        return;
    }

    m_currentOffset += 50;
    m_rawArticles.insert(m_rawArticles.end(), newArticles.begin(), newArticles.end());

    triggerReRank();
}

void MainWindow::triggerReRank()
{
    ui->progressBar->setVisible(true);

    CBibAnalyzer* analyzer = m_analyzer;
    std::string query = m_currentQuery.toStdString();
    auto rawCopy = m_rawArticles;
    RankingMode mode = m_rankingMode;

    m_reRankWatcher->setFuture(QtConcurrent::run([analyzer, query, rawCopy, mode]() -> std::vector<CArticle>
    {
        if (mode == RankByModel)
            return analyzer->RankArticles(query, rawCopy, rawCopy.size());
        else
            return analyzer->RankArticlesTFIDF(query, rawCopy, rawCopy.size());
    }));
}

void MainWindow::onReRankFinished()
{
    auto ranked = m_reRankWatcher->result();
    int scrollPos = ui->articlesList->verticalScrollBar()->value();

    m_articles.clear();
    for (const auto& article : ranked)
        m_articles.push_back(toArticleData(article));

    applySort();

    auto* bar = ui->articlesList->verticalScrollBar();
    bar->blockSignals(true);
    bar->setValue(scrollPos);
    bar->blockSignals(false);

    ui->progressBar->setVisible(false);
    ui->searchButton->setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void MainWindow::onScrollChanged(int value)
{
    if (m_isLoadingMore || m_watcher->isRunning() || m_reRankWatcher->isRunning())
        return;

    auto* bar = ui->articlesList->verticalScrollBar();
    if (bar->maximum() <= 0 || value < bar->maximum() - 7)
        return;

    m_isLoadingMore = true;
    ui->progressBar->setVisible(true);
    QApplication::setOverrideCursor(Qt::WaitCursor);

    int offset = m_currentOffset;
    ArticleSource source = m_currentSource;
    QString query = m_currentQuery;
    auto future = QtConcurrent::run([this, offset, source, query]()
    {
        auto src = createSource(source);
        return src->GetArticles(query.toStdString(), offset);
    });

    m_loadMoreWatcher->setFuture(future);
}

void MainWindow::onSortChanged(int)
{
    applySort();
}

void MainWindow::onRankingModeChanged(int index)
{
    m_rankingMode = (index == 0) ? RankByModel : RankByTFIDF;

    if (m_rawArticles.empty() || m_reRankWatcher->isRunning() || m_watcher->isRunning())
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    triggerReRank();
}

void MainWindow::onArticleSelected(QListWidgetItem* current, QListWidgetItem*)
{
    if (!current)
        return;

    auto data = current->data(Qt::UserRole).value<ArticleData>();
    if (!data.title.isEmpty())
        showDetail(data);
}

void MainWindow::onSavedSortChanged(int)
{
    loadSavedArticles();
}

void MainWindow::onSavedArticleSelected(QListWidgetItem* current, QListWidgetItem*)
{
    if (!current) return;
    auto data = current->data(Qt::UserRole).value<ArticleData>();
    if (!data.title.isEmpty())
        showDetail(data, true);
}

void MainWindow::onExportRequested(ArticleData data, ExportFormat format)
{
    auto* dialog = new ExportDialog(data, format, this);
    dialog->show();
}
