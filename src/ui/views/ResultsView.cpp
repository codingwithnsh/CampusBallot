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
#include <QPainterPath>
#include <QLocale>
#include <QScrollArea>
#include <QDebug> // For logging
#include <algorithm>

namespace Ballot::UI {
namespace {

class ResultsChartWidget final : public QFrame {
public:
    explicit ResultsChartWidget(QWidget* parent = nullptr)
        : QFrame(parent) {
        setMinimumHeight(320);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
        setAccessibleName("Vote distribution chart");
    }

    void setResults(const QList<Core::ElectionResult>& results, int totalVotes) {
        m_results = results;
        m_totalVotes = totalVotes;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QFrame::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const QRectF card = rect().adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath cardPath;
        cardPath.addRoundedRect(card, 12, 12);
        painter.fillPath(cardPath, QColor("#2a2a3e"));

        const int margin = 24;
        QRectF content = rect().adjusted(margin, margin, -margin, -margin);

        QFont titleFont = painter.font();
        titleFont.setPointSize(14);
        titleFont.setBold(true);
        painter.setFont(titleFont);
        painter.setPen(QColor("#e0e0e0"));
        painter.drawText(content.left(), content.top(), content.width(), 26, Qt::AlignLeft | Qt::AlignVCenter, "Vote Distribution");

        QFont captionFont = painter.font();
        captionFont.setPointSize(9);
        captionFont.setBold(false);
        painter.setFont(captionFont);
        painter.setPen(QColor("#9a9ab0"));
        painter.drawText(content.left(), content.top() + 28, content.width(), 20, Qt::AlignLeft | Qt::AlignVCenter,
                         m_totalVotes > 0
                             ? QString("%1 total votes across %2 candidate%3")
                                   .arg(QLocale().toString(m_totalVotes))
                                   .arg(m_results.size())
                                   .arg(m_results.size() == 1 ? "" : "s")
                             : "No votes have been recorded for this election yet.");

        QRectF chartArea = content.adjusted(0, 68, 0, 0);
        if (m_results.isEmpty() || m_totalVotes <= 0) {
            painter.setPen(QColor("#6f6f86"));
            painter.drawText(chartArea, Qt::AlignCenter, "Results will appear here once votes are counted.");
            return;
        }

        int maxVotes = 1;
        for (const auto& result : m_results) {
            maxVotes = std::max(maxVotes, result.voteCount);
        }

        const int rowHeight = 46;
        const int gap = 12;
        const int visibleRows = std::max(1, static_cast<int>(chartArea.height()) / (rowHeight + gap));
        const int rows = std::min(static_cast<int>(m_results.size()), visibleRows);
        const qreal labelWidth = std::min<qreal>(220, chartArea.width() * 0.34);
        const qreal barStart = chartArea.left() + labelWidth + 16;
        const qreal barMaxWidth = std::max<qreal>(40, chartArea.right() - barStart - 92);

        QFont labelFont = painter.font();
        labelFont.setPointSize(10);
        labelFont.setBold(true);
        QFont valueFont = painter.font();
        valueFont.setPointSize(10);
        valueFont.setBold(false);

        for (int i = 0; i < rows; ++i) {
            const auto& result = m_results.at(i);
            const qreal y = chartArea.top() + i * (rowHeight + gap);
            const qreal ratio = maxVotes > 0 ? static_cast<qreal>(result.voteCount) / maxVotes : 0;
            const qreal barWidth = std::max<qreal>(result.voteCount > 0 ? 4 : 0, barMaxWidth * ratio);

            QRectF labelRect(chartArea.left(), y, labelWidth, rowHeight);
            painter.setFont(labelFont);
            painter.setPen(QColor("#ffffff"));
            painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter,
                             painter.fontMetrics().elidedText(result.candidateName, Qt::ElideRight, static_cast<int>(labelRect.width())));

            QColor accent = i == 0 ? QColor("#38bdf8") : QColor("#0078d4");
            QColor track("#1e1e34");
            QRectF trackRect(barStart, y + 9, barMaxWidth, 18);
            QRectF barRect(barStart, y + 9, barWidth, 18);

            QPainterPath trackPath;
            trackPath.addRoundedRect(trackRect, 9, 9);
            painter.fillPath(trackPath, track);

            if (barWidth > 0) {
                QPainterPath barPath;
                barPath.addRoundedRect(barRect, 9, 9);
                painter.fillPath(barPath, accent);
            }

            painter.setFont(valueFont);
            painter.setPen(QColor("#d6d6e8"));
            painter.drawText(QRectF(barStart, y + 28, barMaxWidth, 16),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             QString("%1 • %2%")
                                 .arg(QLocale().toString(result.voteCount))
                                 .arg(QString::number(result.percentage, 'f', 1)));
        }

        if (m_results.size() > rows) {
            painter.setFont(captionFont);
            painter.setPen(QColor("#9a9ab0"));
            painter.drawText(QRectF(chartArea.left(), chartArea.bottom() - 18, chartArea.width(), 18),
                             Qt::AlignRight | Qt::AlignVCenter,
                             QString("+%1 more candidate%2 in the detailed table")
                                 .arg(m_results.size() - rows)
                                 .arg((m_results.size() - rows) == 1 ? "" : "s"));
        }
    }

private:
    QList<Core::ElectionResult> m_results;
    int m_totalVotes = 0;
};

} // namespace

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
            background-color: #25253a; color: #ffffff; border: none;
            border-radius: 8px; padding: 8px 12px; font-size: 14px;
        }
        QComboBox::drop-down { border: 0px; }
        QComboBox::down-arrow { width: 16px; height: 16px; }
        QComboBox:hover { border: none; }
        QComboBox QAbstractItemView {
            background-color: #25253a; color: #ffffff; selection-background-color: #0078d4;
            border: none; border-radius: 8px;
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
    if (auto* chart = static_cast<ResultsChartWidget*>(m_chartWidget)) {
        chart->setResults(results, m_viewModel->totalVotes());
    }
    qDebug() << "ResultsView: UI update complete.";
    updateExportAvailability();
}

/**
 * @brief Creates a chart widget for displaying vote distribution.
 * @return The chart widget.
 */
QWidget* ResultsView::createChartWidget() {
    return new ResultsChartWidget(this);
}

} // namespace Ballot::UI
