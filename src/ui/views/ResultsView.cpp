#include "ResultsView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QPainter>
#include <QLocale>
#include <QScrollArea>

namespace Ballot::UI {

ResultsView::ResultsView(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void ResultsView::setViewModel(ViewModels::ResultsViewModel* vm) {
    m_viewModel = vm;
    if (vm) {
        connect(vm, &ViewModels::ResultsViewModel::resultsChanged, this, &ResultsView::updateUi);
        connect(vm, &ViewModels::ResultsViewModel::exportCompleted, this, [this](const QString& path) {
            // Toast notification would go here
        });
    }
}

void ResultsView::setupUi() {
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    // Header
    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("Election Results");
    title->setObjectName("title");
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    m_electionSelector = new QComboBox(this);
    m_electionSelector->setMinimumWidth(250);
    headerLayout->addWidget(m_electionSelector);

    mainLayout->addLayout(headerLayout);

    // Summary stats
    auto *summaryLayout = new QHBoxLayout();
    summaryLayout->setSpacing(16);

    auto *turnoutFrame = new QFrame(this);
    turnoutFrame->setObjectName("statCard");
    auto *turnoutVbox = new QVBoxLayout(turnoutFrame);
    auto *tLabel = new QLabel("Voter Turnout");
    tLabel->setObjectName("statLabel");
    m_turnoutLabel = new QLabel("0%");
    m_turnoutLabel->setObjectName("statValue");
    turnoutVbox->addWidget(tLabel);
    turnoutVbox->addWidget(m_turnoutLabel);
    summaryLayout->addWidget(turnoutFrame);

    auto *votesFrame = new QFrame(this);
    votesFrame->setObjectName("statCard");
    auto *votesVbox = new QVBoxLayout(votesFrame);
    auto *vLabel = new QLabel("Total Votes Cast");
    vLabel->setObjectName("statLabel");
    m_totalVotesLabel = new QLabel("0");
    m_totalVotesLabel->setObjectName("statValue");
    votesVbox->addWidget(vLabel);
    votesVbox->addWidget(m_totalVotesLabel);
    summaryLayout->addWidget(votesFrame);

    mainLayout->addLayout(summaryLayout);

    // Chart area
    m_chartWidget = createChartWidget();
    mainLayout->addWidget(m_chartWidget);

    // Results table
    auto *tableSection = new QFrame(this);
    tableSection->setObjectName("card");
    auto *tableLayout = new QVBoxLayout(tableSection);

    auto *tableHeader = new QLabel("Detailed Results");
    tableHeader->setObjectName("sectionTitle");
    tableLayout->addWidget(tableHeader);

    m_resultsTable = new QTableWidget(this);
    m_resultsTable->setColumnCount(4);
    m_resultsTable->setHorizontalHeaderLabels({"Candidate", "Party", "Votes", "Percentage"});
    m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTable->setAlternatingRowColors(true);
    m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableLayout->addWidget(m_resultsTable);

    mainLayout->addWidget(tableSection);

    // Export buttons
    auto *exportLayout = new QHBoxLayout();
    auto *exportLabel = new QLabel("Export Results:");
    exportLabel->setStyleSheet("font-weight: 600; color: #e0e0e0; background: transparent;");
    exportLayout->addWidget(exportLabel);

    m_exportCsvBtn = new QPushButton("CSV");
    m_exportCsvBtn->setObjectName("ghostButton");
    m_exportJsonBtn = new QPushButton("JSON");
    m_exportJsonBtn->setObjectName("ghostButton");
    m_exportPdfBtn = new QPushButton("PDF");
    m_exportPdfBtn->setObjectName("ghostButton");

    connect(m_exportCsvBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, "Export CSV", "results.csv", "CSV (*.csv)");
        if (!path.isEmpty() && m_viewModel) m_viewModel->exportResults(path, "csv");
    });
    connect(m_exportJsonBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, "Export JSON", "results.json", "JSON (*.json)");
        if (!path.isEmpty() && m_viewModel) m_viewModel->exportResults(path, "json");
    });

    exportLayout->addWidget(m_exportCsvBtn);
    exportLayout->addWidget(m_exportJsonBtn);
    exportLayout->addWidget(m_exportPdfBtn);
    exportLayout->addStretch();
    mainLayout->addLayout(exportLayout);

    mainLayout->addStretch();

    scrollArea->setWidget(content);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    connect(m_electionSelector, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        if (m_viewModel && !text.isEmpty()) {
            QString id = m_electionSelector->currentData().toString();
            m_viewModel->setElection(id);
        }
    });
}

void ResultsView::updateUi() {
    if (!m_viewModel) return;

    // Update election selector
    m_electionSelector->blockSignals(true);
    m_electionSelector->clear();
    auto elections = m_viewModel->getElections();
    for (const auto& e : elections) {
        m_electionSelector->addItem(e.title, e.id);
    }
    m_electionSelector->blockSignals(false);

    m_turnoutLabel->setText(QString::number(m_viewModel->turnout(), 'f', 1) + "%");
    m_totalVotesLabel->setText(QLocale().toString(m_viewModel->totalVotes()));

    // Update table
    auto results = m_viewModel->results();
    m_resultsTable->setRowCount(results.size());
    for (int i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        m_resultsTable->setItem(i, 0, new QTableWidgetItem(r.candidateName));
        m_resultsTable->setItem(i, 1, new QTableWidgetItem(r.party));
        m_resultsTable->setItem(i, 2, new QTableWidgetItem(QLocale().toString(r.voteCount)));
        m_resultsTable->setItem(i, 3, new QTableWidgetItem(QString::number(r.percentage, 'f', 1) + "%"));
    }
}

QWidget* ResultsView::createChartWidget() {
    auto *widget = new QFrame(this);
    widget->setObjectName("card");
    widget->setMinimumHeight(300);

    auto *layout = new QVBoxLayout(widget);
    auto *header = new QLabel("Results Chart");
    header->setObjectName("sectionTitle");
    layout->addWidget(header);

    // Placeholder for chart - in production use QtCharts
    auto *chartPlaceholder = new QFrame(widget);
    chartPlaceholder->setStyleSheet("background-color: #1e1e34; border-radius: 8px; border: 1px solid #2d2d44;");
    chartPlaceholder->setMinimumHeight(250);

    auto *chartLayout = new QVBoxLayout(chartPlaceholder);
    auto *chartLabel = new QLabel("Live results chart will render here using QtCharts");
    chartLabel->setStyleSheet("color: #9a9ab0; font-size: 14px; background: transparent;");
    chartLabel->setAlignment(Qt::AlignCenter);
    chartLayout->addWidget(chartLabel);

    layout->addWidget(chartPlaceholder);
    return widget;
}

} // namespace Ballot::UI
