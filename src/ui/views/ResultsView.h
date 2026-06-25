#pragma once

#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QListWidget>
#include "src/ui/viewmodels/ResultsViewModel.h"

namespace Ballot::UI {

class ResultsView : public QWidget {
    Q_OBJECT
public:
    explicit ResultsView(QWidget *parent = nullptr);
    void setViewModel(ViewModels::ResultsViewModel* vm);

private:
    void setupUi();
    void updateUi();
    QWidget* createChartWidget();

    ViewModels::ResultsViewModel* m_viewModel = nullptr;
    QComboBox* m_electionSelector;
    QLabel* m_turnoutLabel;
    QLabel* m_totalVotesLabel;
    QTableWidget* m_resultsTable;
    QWidget* m_chartWidget;
    QPushButton* m_exportCsvBtn;
    QPushButton* m_exportJsonBtn;
    QPushButton* m_exportPdfBtn;
};

} // namespace Ballot::UI
