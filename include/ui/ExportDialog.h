#pragma once

#include "ui/ArticleItemWidget.h"
#include <QDialog>

class ExportDialog : public QDialog {
    Q_OBJECT
public:
    explicit ExportDialog(const ArticleData& data, ExportFormat format, QWidget* parent = nullptr);
};