#include "ResultsView.h"
#include "src/ui/components/ToastNotification.h"
#include "src/modules/election/ElectionManager.h" // For getting elections
#include "src/modules/audit/AuditManager.h" // For audit logging
#include "src/modules/auth/AuthManager.h"
#include "src/modules/auth/RBACManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QPainter> // For custom painting if needed
#include <QLocale>
#include <QScrollArea>
#include <QDebug> // For logging

// QtCharts includes (if actually used, otherwise keep as placeholder)
// #include <QtCharts/QChartView>
// #include <QtCharts/QBarSeries>
// #include <QtCharts/QBarSet>
// #include <QtCharts/QLegend>
// #include <QtCharts/QBarCategoryAxis>
// #include <QtCharts/QValueAxis>

namespace Ballot::UI {

ResultsView::ResultsView(QWidget *parent) : QWidget(parent) {
    setupUi();
    connect(&Auth::AuthManager::instance(), &Auth::AuthManager::authStateChanged, this, &ResultsView::updateExportAvailability);
    connect(&Auth::AuthManager::instance(), &Auth::AuthManager::loginSuccessful, this, [this](const QString&) { updateExportAvailability(); });
    connect(&Auth::AuthManager::instance(), &Auth::AuthManager::logoutOccurred, this, &ResultsView::updateExportAvailability);
    qDebug() << "ResultsView: Initialized.";
}

/**
 * @brief Sets the ResultsViewModel for this view.
 * Connects signals from the ViewModel to update the UI.
 * @param vm The ResultsViewModel instance.
 */
void ResultsView::setViewModel(ViewModels::ResultsViewModel* vm) {
    if (m_viewModel == vm) return; // Avoid reconnecting if same VM
    if (m_viewModel) {
        // Disconnect old connections if a ViewModel was already set
        disconnect(m_viewModel, nullptr, this, nullptr);
    }

    m_viewModel = vm;
    if (m_viewModel) {
        connect(m_viewModel, &ViewModels::ResultsViewModel::resultsChanged, this, &ResultsView::updateUi);
        connect(m_viewModel, &ViewModels::ResultsViewModel::exportCompleted, this, [this](const QString& path) {
            ToastNotification::show(this, "Results exported to: " + path, ToastNotification::Success);
            qInfo() << "ResultsView: Export completed to" << path;
        });
        connect(m_viewModel, &ViewModels::ResultsViewModel::errorOccurred, this, [this](const QString& error) {
            ToastNotification::show(this, error, ToastNotification::Error);
            qWarning() << "ResultsView: ViewModel error:" << error;
        });
        updateUi(); // Initial UI update when view model is set
        qDebug() << "ResultsView: ViewModel set and signals connected.";
    } else {
        qWarning() << "ResultsView: Attempted to set null ViewModel.";
    }
}

/**
 * @brief Sets up the user interface for the results view.
 */
void ResultsView::setupUi() {
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    // --- Header ---
    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("Election Results", this);
    title->setObjectName("title");
    title->setStyleSheet("font-size: 32px; font-weight: 700; color: #e0e0e0;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    m_electionSelector = new QComboBox(this);
    m_electionSelector->setMinimumWidth(250);
    m_electionSelector->setStyleSheet(R"(
        QComboBox {
            background-color: #25253a; color: #ffffff; border: 1px solid #3d3d5c;
            border-radius: 8px; padding: 8px 12px; font-size: 14px;
        }
        QComboBox::drop-down { border: 0px; }
        QComboBox::down-arrow { width: 16px; height: 16px; }
        QComboBox:hover { border: 1px solid #6b7280; }
        QComboBox QAbstractItemView {
            background-color: #25253a; color: #ffffff; selection-background-color: #0078d4;
            border: 1px solid #3d3d5c; border-radius: 8px;
        }
    )");
    headerLayout->addWidget(m_electionSelector);

    mainLayout->addLayout(headerLayout);

    // --- Summary stats ---
    auto *summaryLayout = new QHBoxLayout();
    summaryLayout->setSpacing(16);

    auto createSummaryCard = [&](const QString& cardTitle, QLabel*& valueLabel, const QString& defaultVal, const QString& accentColor) {
        auto *frame = new QFrame(this);
        frame->setObjectName("statCard");
        frame->setStyleSheet("QFrame#statCard { background-color: #2a2a3e; border-radius: 12px; padding: 16px; }");
        auto *vbox = new QVBoxLayout(frame);
        vbox->setContentsMargins(0,0,0,0);
        vbox->setSpacing(4);

        auto *tLabel = new QLabel(cardTitle, frame);
        tLabel->setStyleSheet("font-size: 12px; color: #9a9ab0; font-weight: 500; background: transparent;");
        valueLabel = new QLabel(defaultVal, frame);
        valueLabel->setStyleSheet(QString("font-size: 28px; font-weight: 700; color: %1; background: transparent;").arg(accentColor));
        vbox->addWidget(tLabel);
        vbox->addWidget(valueLabel);
        return frame;
    };

    summaryLayout->addWidget(createSummaryCard("Voter Turnout", m_turnoutLabel, "0%", "#0078d4"));
    summaryLayout->addWidget(createSummaryCard("Total Votes Cast", m_totalVotesLabel, "0", "#2e7d32"));

    mainLayout->addLayout(summaryLayout);

    // --- Chart area ---
    m_chartWidget = createChartWidget();
    mainLayout->addWidget(m_chartWidget);

    // --- Results table ---
    auto *tableSection = new QFrame(this);
    tableSection->setObjectName("card");
    tableSection->setStyleSheet("QFrame#card { background-color: #2a2a3e; border-radius: 12px; padding: 16px; }");
    auto *tableLayout = new QVBoxLayout(tableSection);

    auto *tableHeader = new QLabel("Detailed Results", this);
    tableHeader->setObjectName("sectionTitle");
    tableHeader->setStyleSheet("font-size: 18px; font-weight: 600; color: #e0e0e0; margin-bottom: 10px;");
    tableLayout->addWidget(tableHeader);

    m_resultsTable = new QTableWidget(this);
    m_resultsTable->setColumnCount(4);
    m_resultsTable->setHorizontalHeaderLabels({"Candidate", "Party", "Votes", "Percentage"});
    m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTable->setAlternatingRowColors(true);
    m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultsTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e1e34; alternate-background-color: #25253a;
            border: 1px solid #3d3d5c; border-radius: 8px; color: #e0e0e0;
            font-size: 14px;
        }
        QHeaderView::section {
            background-color: #2d2d44; color: #ffffff; padding: 8px;
            border: 1px solid #3d3d5c; font-weight: 600;
        }
        QTableWidget::item { padding: 8px; }
    )");
    tableLayout->addWidget(m_resultsTable);

    mainLayout->addWidget(tableSection);

    // --- Export buttons ---
    auto *exportLayout = new QHBoxLayout();
    auto *exportLabel = new QLabel("Export Results:", this);
    exportLabel->setStyleSheet("font-weight: 600; color: #e0e0e0; background: transparent;");
    exportLayout->addWidget(exportLabel);

    m_exportCsvBtn = new QPushButton("CSV", this);
    m_exportCsvBtn->setObjectName("ghostButton");
    m_exportJsonBtn = new QPushButton("JSON", this);
    m_exportJsonBtn->setObjectName("ghostButton");
    m_exportPdfBtn = new QPushButton("PDF", this);
    m_exportPdfBtn->setObjectName("ghostButton");

    // Common stylesheet for export buttons
    const QString exportBtnStyle = R"(
        QPushButton#ghostButton {
            background-color: transparent; color: #0078d4; border: 1px solid #0078d4;
            border-radius: 8px; padding: 8px 16px; font-size: 14px; font-weight: 600;
        }
        QPushButton#ghostButton:hover { background-color: rgba(0, 120, 212, 0.1); }
        QPushButton#ghostButton:pressed { background-color: rgba(0, 120, 212, 0.2); }
    )";
    m_exportCsvBtn->setStyleSheet(exportBtnStyle);
    m_exportJsonBtn->setStyleSheet(exportBtnStyle);
    m_exportPdfBtn->setStyleSheet(exportBtnStyle);

    connect(m_exportCsvBtn, &QPushButton::clicked, this, [this]() {
        exportResultsAs("csv", "election_results.csv", "CSV (*.csv)");
    });
    connect(m_exportJsonBtn, &QPushButton::clicked, this, [this]() {
        exportResultsAs("json", "election_results.json", "JSON (*.json)");
    });
    connect(m_exportPdfBtn, &QPushButton::clicked, this, [this]() {
        exportResultsAs("pdf", "election_results.pdf", "PDF (*.pdf)");
    });

    exportLayout->addWidget(m_exportCsvBtn);
    exportLayout->addWidget(m_exportJsonBtn);
    exportLayout->addWidget(m_exportPdfBtn);
    exportLayout->addStretch();
    mainLayout->addLayout(exportLayout);

    mainLayout->addStretch(); // Pushes content to the top

    scrollArea->setWidget(content);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    // Connect election selector signal
    connect(m_electionSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (m_viewModel && index >= 0) {
            QString id = m_electionSelector->currentData().toString();
            m_viewModel->setElection(id);
            qDebug() << "ResultsView: Election selected:" << id;
        }
    });
    updateExportAvailability();
    qDebug() << "ResultsView: UI setup complete.";
}

void ResultsView::updateExportAvailability() {
    auto& auth = Auth::AuthManager::instance();
    const bool canExport = auth.isAuthenticated()
        && Auth::RBACManager::instance().hasPermission(auth.currentRole(), Auth::RBACManager::PERM_RESULTS_EXPORT);
    const QString tooltip = canExport ? "Export election results" : "Requires results export permission";

    m_exportCsvBtn->setEnabled(canExport);
    m_exportJsonBtn->setEnabled(canExport);
    m_exportPdfBtn->setEnabled(canExport);
    m_exportCsvBtn->setToolTip(tooltip);
    m_exportJsonBtn->setToolTip(tooltip);
    m_exportPdfBtn->setToolTip(tooltip);
}

void ResultsView::exportResultsAs(const QString& format, const QString& defaultFileName, const QString& fileFilter) {
    if (!Auth::AuthManager::instance().hasPermission(Auth::RBACManager::PERM_RESULTS_EXPORT)) {
        ToastNotification::show(this, "You do not have permission to export results.", ToastNotification::Error);
        Audit::AuditManager::instance().log(Core::AuditAction::PermissionDenied, QString("Denied results export as %1.").arg(format), Auth::AuthManager::instance().currentUserId());
        return;
    }

    if (!m_viewModel) {
        ToastNotification::show(this, "Results are not ready to export.", ToastNotification::Warning);
        return;
    }

    const QString path = QFileDialog::getSaveFileName(this, "Export " + format.toUpper(), defaultFileName, fileFilter);
    if (!path.isEmpty()) {
        m_viewModel->exportResults(path, format);
    }
}

/**
 * @brief Updates the UI elements with data from the ViewModel.
 * This slot is connected to the ViewModel's resultsChanged signal.
 */
void ResultsView::updateUi() {
    if (!m_viewModel) {
        qWarning() << "ResultsView: ViewModel is null during updateUi.";
        return;
    }
    qDebug() << "ResultsView: Updating UI from ViewModel.";

    // Store current election ID to re-select it after updating the list
    QString currentElectionId = m_viewModel->currentElectionId();

    m_electionSelector->blockSignals(true); // Block signals to prevent re-triggering setElection
    m_electionSelector->clear();
    auto elections = m_viewModel->getElections();
    int currentIndex = -1;
    for (int i = 0; i < elections.size(); ++i) {
        const auto& e = elections[i];
        m_electionSelector->addItem(e.title, e.id);
        if (e.id == currentElectionId) {
            currentIndex = i;
        }
    }
    if (currentIndex != -1) {
        m_electionSelector->setCurrentIndex(currentIndex);
    } else if (!elections.isEmpty()) {
        // If current election not found (e.g., deleted), select the first one
        m_electionSelector->setCurrentIndex(0);
        // This will trigger setElection via the currentIndexChanged signal if not blocked
        // Since we blocked signals, we need to manually call it if the selection changes.
        if (m_viewModel->currentElectionId() != elections.first().id) {
             m_viewModel->setElection(elections.first().id);
        }
    } else {
        // No elections at all
        if (!m_viewModel->currentElectionId().isEmpty()
            || !m_viewModel->results().isEmpty()
            || m_viewModel->totalVotes() != 0) {
            m_viewModel->setElection(""); // Clear view model state
        }
    }
    m_electionSelector->blockSignals(false);

    // Update summary labels
    m_turnoutLabel->setText(QString::number(m_viewModel->turnout(), 'f', 1) + "%");
    m_totalVotesLabel->setText(QLocale().toString(m_viewModel->totalVotes()));

    // Update results table
    auto results = m_viewModel->results();
    m_resultsTable->setRowCount(results.size());
    for (int i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        m_resultsTable->setItem(i, 0, new QTableWidgetItem(r.candidateName));
        m_resultsTable->setItem(i, 1, new QTableWidgetItem(r.party));
        m_resultsTable->setItem(i, 2, new QTableWidgetItem(QLocale().toString(r.voteCount)));
        m_resultsTable->setItem(i, 3, new QTableWidgetItem(QString::number(r.percentage, 'f', 1) + "%"));
    }
    qDebug() << "ResultsView: UI update complete.";
    updateExportAvailability();
}

/**
 * @brief Creates a placeholder widget for displaying charts.
 * @return The chart widget.
 */
QWidget* ResultsView::createChartWidget() {
    auto *widget = new QFrame(this);
    widget->setObjectName("card");
    widget->setStyleSheet("QFrame#card { background-color: #2a2a3e; border-radius: 12px; padding: 16px; }");
    widget->setMinimumHeight(300);

    auto *layout = new QVBoxLayout(widget);
    auto *header = new QLabel("Results Chart", this);
    header->setObjectName("sectionTitle");
    header->setStyleSheet("font-size: 18px; font-weight: 600; color: #e0e0e0; margin-bottom: 10px;");
    layout->addWidget(header);

    // Placeholder for chart - in production use QtCharts
    auto *chartPlaceholder = new QFrame(widget);
    chartPlaceholder->setStyleSheet("background-color: #1e1e34; border-radius: 8px; border: 1px solid #2d2d44;");
    chartPlaceholder->setMinimumHeight(250);

    auto *chartLayout = new QVBoxLayout(chartPlaceholder);
    auto *chartLabel = new QLabel("Live results chart will render here using QtCharts", chartPlaceholder);
    chartLabel->setStyleSheet("color: #9a9ab0; font-size: 14px; background: transparent;");
    chartLabel->setAlignment(Qt::AlignCenter);
    chartLayout->addWidget(chartLabel);

    layout->addWidget(chartPlaceholder);
    return widget;
}

} // namespace Ballot::UI
