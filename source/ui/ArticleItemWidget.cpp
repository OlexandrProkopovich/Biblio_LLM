#include "ui/ArticleItemWidget.h"

#include <QDesktopServices>
#include <QUrl>

ArticleItemWidget::ArticleItemWidget(const ArticleData& data, WidgetMode mode, QWidget* parent)
    : QWidget(parent), m_data(data)
{
    auto* mainLayout = new QVBoxLayout(this);
    auto* topRow = new QHBoxLayout;
    auto* btnRow = new QHBoxLayout;

    m_titleLabel = new QLabel("<b>" + data.title + "</b>");
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_titleLabel->setOpenExternalLinks(true);
    m_titleLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    m_authorsLabel = new QLabel(data.authors + " | " + data.published);
    m_authorsLabel->setWordWrap(true);
    m_abstractLabel = new QLabel(data.abstract.left(200) + "...");
    m_abstractLabel->setWordWrap(true);

    auto* actionBtn = new QPushButton(mode == SavedMode ? "Delete" : "Save");
    actionBtn->setFixedWidth(90);

    auto* plainBtn = new QPushButton("PlainText");
    auto* risBtn = new QPushButton("RIS");
    auto* bibtexBtn = new QPushButton("BibTeX");
    auto* pdfBtn = new QPushButton("PDF");

    topRow->addWidget(m_titleLabel, 1);
    topRow->addWidget(actionBtn, 0);
    topRow->addWidget(pdfBtn, 0);

    connect(pdfBtn, &QPushButton::clicked, this, [this] 
    {
        QString pdfUrl = m_data.url;
        pdfUrl.replace("abs", "pdf");
        QDesktopServices::openUrl(QUrl(pdfUrl));
    });

    btnRow->addWidget(plainBtn);
    btnRow->addWidget(risBtn);
    btnRow->addWidget(bibtexBtn);
    btnRow->addStretch();

    mainLayout->addLayout(topRow);
    mainLayout->addWidget(m_authorsLabel);
    mainLayout->addWidget(m_abstractLabel);
    mainLayout->addLayout(btnRow);

    if (mode == SavedMode)
        connect(actionBtn, &QPushButton::clicked, this, [this] { emit deleteRequested(m_data); });
    else
        connect(actionBtn, &QPushButton::clicked, this, [this] { emit saveRequested(m_data); });

    connect(plainBtn, &QPushButton::clicked, this, [this] { emit exportRequested(m_data, ExportFormat::PlainText); });
    connect(risBtn, &QPushButton::clicked, this, [this] { emit exportRequested(m_data, ExportFormat::RIS); });
    connect(bibtexBtn, &QPushButton::clicked, this, [this] { emit exportRequested(m_data, ExportFormat::BibTeX); });
}