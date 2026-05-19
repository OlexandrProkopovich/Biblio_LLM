#include "ui/ExportDialog.h"
#include <QTextEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QClipboard>

struct Field
{
    QString label;
    QString key;
    QString value;
    bool checked;
};

ExportDialog::ExportDialog(const ArticleData& data, ExportFormat format, QWidget* parent)
    : QDialog(parent)
{
    setMinimumSize(700, 400);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);

    QList<Field> fields;

    if (format == ExportFormat::BibTeX) 
    {
        setWindowTitle("BibTeX Export");
        fields = 
        {
            {"title",    "title",    data.title,                true},
            {"author",   "author",   data.authors,              true},
            {"year",     "year",     data.published.left(4),    true},
            {"url",      "url",      data.url,                  true},
            {"abstract", "abstract", data.abstract,             false},
            {"journal",  "journal",  data.journal,              false},
            {"doi",      "doi",      data.doi,                  false},
        };
    }
    else if (format == ExportFormat::RIS) 
    {
        setWindowTitle("RIS Export");
        fields = 
        {
            {"Title",    "TI",  data.title,             true},
            {"Author",   "AU",  data.authors,           true},
            {"Year",     "PY",  data.published.left(4), true},
            {"URL",      "UR",  data.url,               true},
            {"Abstract", "AB",  data.abstract,          false},
            {"Journal",  "JO",  data.journal,           false},
            {"DOI",      "DO",  data.doi,               false},
        };
    }
    else 
    {
        setWindowTitle("PlainText Export");
        fields = 
        {
            {"Title",    "Title",    data.title,             true},
            {"Authors",  "Authors",  data.authors,           true},
            {"Year",     "Year",     data.published.left(4), true},
            {"URL",      "URL",      data.url,               true},
            {"Abstract", "Abstract", data.abstract,          false},
            {"Journal",  "Journal",  data.journal,           false},
            {"DOI",      "DOI",      data.doi,               false},
        };
    }

    auto* mainLayout = new QHBoxLayout(this);
    auto* leftLayout = new QVBoxLayout;
    auto* rightLayout = new QVBoxLayout;

    auto* textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    auto* copyBtn = new QPushButton("Copy", this);

    QList<QCheckBox*> checkboxes;
    for (const Field& f : fields) 
    {
        auto* cb = new QCheckBox(f.label, this);
        cb->setChecked(f.checked);
        rightLayout->addWidget(cb);
        checkboxes.append(cb);
    }
    rightLayout->addStretch();

    auto rebuild = [=]() 
    {
        QString text;
        if (format == ExportFormat::BibTeX) 
        {
            QString key = data.authors.split(" ").last() + data.published.left(4);
            text = "@article{" + key + ",\n";
            for (int i = 0; i < fields.size(); i++)
                if (checkboxes[i]->isChecked() && !fields[i].value.isEmpty())
                    text += "  " + fields[i].key + " = {" + fields[i].value + "},\n";

            text += "}\n";
        }
        else if (format == ExportFormat::RIS)
        {
            text = "TY  - JOUR\n";
            for (int i = 0; i < fields.size(); i++)
                if (checkboxes[i]->isChecked() && !fields[i].value.isEmpty())
                    text += fields[i].key + "  - " + fields[i].value + "\n";
            text += "ER  -\n";
        }
        else {
            for (int i = 0; i < fields.size(); i++)
                if (checkboxes[i]->isChecked() && !fields[i].value.isEmpty())
                    text += fields[i].key + ": " + fields[i].value + "\n";
        }
        textEdit->setPlainText(text);
    };

    rebuild();

    for (auto* cb : checkboxes)
        connect(cb, &QCheckBox::toggled, this, rebuild);

    connect(copyBtn, &QPushButton::clicked, this, [=] {
        QApplication::clipboard()->setText(textEdit->toPlainText());
        copyBtn->setText("Copied!");
        });

    leftLayout->addWidget(textEdit);
    leftLayout->addWidget(copyBtn);
    mainLayout->addLayout(leftLayout, 3);
    mainLayout->addLayout(rightLayout, 1);
}