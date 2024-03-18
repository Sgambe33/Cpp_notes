// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "searchresultdelegate.h"
#include "zoomselector.h"

#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfPageNavigator>
#include <QPdfPageSelector>
#include <QPdfSearchModel>
#include <QShortcut>
#include <QStandardPaths>
#include <QtMath>
#include <QProcess>

const qreal zoomMultiplier = qSqrt(2.0);
QProcess *myProcess = new QProcess();


Q_LOGGING_CATEGORY(lcExample, "qt.examples.pdfviewer")

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_zoomSelector(new ZoomSelector(this))
    , m_pageSelector(new QPdfPageSelector(this))
    , m_searchModel(new QPdfSearchModel(this))
    , m_searchField(new QLineEdit(this))
    , m_document(new QPdfDocument(this))
{
    ui->setupUi(this);

    m_zoomSelector->setMaximumWidth(150);
    ui->mainToolBar->insertWidget(ui->actionZoom_In, m_zoomSelector);

    ui->mainToolBar->insertWidget(ui->actionForward, m_pageSelector);
    connect(m_pageSelector, &QPdfPageSelector::currentPageChanged, this, &MainWindow::pageSelected);
    m_pageSelector->setDocument(m_document);
    auto nav = ui->pdfView_2->pageNavigator();
    connect(nav, &QPdfPageNavigator::currentPageChanged, m_pageSelector, &QPdfPageSelector::setCurrentPage);
    connect(nav, &QPdfPageNavigator::backAvailableChanged, ui->actionBack, &QAction::setEnabled);
    connect(nav, &QPdfPageNavigator::forwardAvailableChanged, ui->actionForward, &QAction::setEnabled);

    connect(m_zoomSelector, &ZoomSelector::zoomModeChanged, ui->pdfView_2, &QPdfView::setZoomMode);
    connect(m_zoomSelector, &ZoomSelector::zoomFactorChanged, ui->pdfView_2, &QPdfView::setZoomFactor);
    m_zoomSelector->reset();

    QPdfBookmarkModel *bookmarkModel = new QPdfBookmarkModel(this);
    bookmarkModel->setDocument(m_document);

    ui->bookmarkView->setModel(bookmarkModel);
    connect(ui->bookmarkView, &QAbstractItemView::activated, this, &MainWindow::bookmarkSelected);

    ui->thumbnailsView->setModel(m_document->pageModel());

    m_searchModel->setDocument(m_document);
    ui->pdfView_2->setSearchModel(m_searchModel);
    ui->searchToolBar->insertWidget(ui->actionFindPrevious, m_searchField);
    connect(new QShortcut(QKeySequence::Find, this), &QShortcut::activated, this, [this]() {
        m_searchField->setFocus(Qt::ShortcutFocusReason);
    });
    m_searchField->setPlaceholderText(tr("Find in document"));
    m_searchField->setMaximumWidth(400);
    connect(m_searchField, &QLineEdit::textEdited, this, [this](const QString &text) {
        m_searchModel->setSearchString(text);
        ui->tabWidget->setCurrentWidget(ui->searchResultsTab);
    });
    ui->searchResultsView->setModel(m_searchModel);
    ui->searchResultsView->setItemDelegate(new SearchResultDelegate(this));
    connect(ui->searchResultsView->selectionModel(), &QItemSelectionModel::currentChanged,this, &MainWindow::searchResultSelected);

    ui->pdfView_2->setDocument(m_document);

    connect(ui->pdfView_2, &QPdfView::zoomFactorChanged,m_zoomSelector, &ZoomSelector::setZoomFactor);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::open(const QUrl &docLocation)
{
    if (docLocation.isLocalFile()) {
        m_document->load(docLocation.toLocalFile());
        pageSelected(0);
        ui->pdfView_2->setPageMode(QPdfView::PageMode::MultiPage);
        ui->pdfView_2->setZoomMode(QPdfView::ZoomMode::FitToWidth);
    } else {
        const QString message = tr("%1 is not a valid local file").arg(docLocation.toString());
        qCDebug(lcExample).noquote() << message;
        QMessageBox::critical(this, tr("Failed to open"), message);
    }
    qCDebug(lcExample) << docLocation;
    //C:/Users/Cosimo/Desktop/GitHub/untitled/
    //E:/GITHUB_REPOS/untitled/
    QString program = "C:/Users/Cosimo/Desktop/GitHub/untitled/typst.exe";
    QStringList arguments;
    arguments << "watch" << "C:/Users/Cosimo/Desktop/GitHub/untitled/test.typ";

    myProcess->start(program, arguments);
}

void MainWindow::bookmarkSelected(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    const int page = index.data(int(QPdfBookmarkModel::Role::Page)).toInt();
    const qreal zoomLevel = index.data(int(QPdfBookmarkModel::Role::Level)).toReal();
    ui->pdfView_2->pageNavigator()->jump(page, {}, zoomLevel);
}

void MainWindow::pageSelected(int page)
{
    auto nav = ui->pdfView_2->pageNavigator();
    nav->jump(page, {}, nav->currentZoom());
    const auto documentTitle = m_document->metaData(QPdfDocument::MetaDataField::Title).toString();
    setWindowTitle(!documentTitle.isEmpty() ? documentTitle : QStringLiteral("PDF Viewer"));
    setWindowTitle(tr("%1: page %2 (%3 of %4)")
                       .arg(documentTitle.isEmpty() ? u"PDF Viewer"_qs : documentTitle,
                            m_pageSelector->currentPageLabel(), QString::number(page + 1), QString::number(m_document->pageCount())));
}

void MainWindow::searchResultSelected(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    if (!current.isValid())
        return;

    const int page = current.data(int(QPdfSearchModel::Role::Page)).toInt();
    const QPointF location = current.data(int(QPdfSearchModel::Role::Location)).toPointF();
    ui->pdfView_2->pageNavigator()->jump(page, location);
    ui->pdfView_2->setCurrentSearchResultIndex(current.row());
}

void MainWindow::on_actionOpen_triggered()
{
    if (m_fileDialog == nullptr) {
        m_fileDialog = new QFileDialog(this, tr("Choose a PDF"),
                                       QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        m_fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
        m_fileDialog->setMimeTypeFilters({"application/pdf"});
    }

    if (m_fileDialog->exec() == QDialog::Accepted) {
        const QUrl toOpen = m_fileDialog->selectedUrls().constFirst();
        if (toOpen.isValid())
            open(toOpen);
    }
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::quit();
}

void MainWindow::terminate(){
    myProcess->kill();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About PdfViewer"),
                       tr("An example using QPdfDocument"));
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::on_actionZoom_In_triggered()
{
    ui->pdfView_2->setZoomFactor(ui->pdfView_2->zoomFactor() * zoomMultiplier);
}

void MainWindow::on_actionZoom_Out_triggered()
{
    ui->pdfView_2->setZoomFactor(ui->pdfView_2->zoomFactor() / zoomMultiplier);
}

void MainWindow::on_actionPrevious_Page_triggered()
{
    auto nav = ui->pdfView_2->pageNavigator();
    nav->jump(nav->currentPage() - 1, {}, nav->currentZoom());
}

void MainWindow::on_actionNext_Page_triggered()
{
    auto nav = ui->pdfView_2->pageNavigator();
    nav->jump(nav->currentPage() + 1, {}, nav->currentZoom());
}

void MainWindow::on_thumbnailsView_activated(const QModelIndex &index)
{
    auto nav = ui->pdfView_2->pageNavigator();
    nav->jump(index.row(), {}, nav->currentZoom());
}

void MainWindow::on_actionContinuous_triggered()
{
    /*ui->pdfView_2->setPageMode(ui->actionContinuous->isChecked() ?
                                 QPdfView::PageMode::MultiPage :
                                 QPdfView::PageMode::SinglePage);*/
    ui->pdfView_2->setPageMode(QPdfView::PageMode::MultiPage);
}

void MainWindow::on_actionBack_triggered()
{
    ui->pdfView_2->pageNavigator()->back();
}

void MainWindow::on_actionForward_triggered()
{
    ui->pdfView_2->pageNavigator()->forward();
}

void MainWindow::on_actionFindNext_triggered()
{
    int next = ui->searchResultsView->currentIndex().row() + 1;
    if (next >= m_searchModel->rowCount({}))
        next = 0;
    ui->searchResultsView->setCurrentIndex(m_searchModel->index(next));
}

void MainWindow::on_actionFindPrevious_triggered()
{
    int prev = ui->searchResultsView->currentIndex().row() - 1;
    if (prev < 0)
        prev = m_searchModel->rowCount({}) - 1;
    ui->searchResultsView->setCurrentIndex(m_searchModel->index(prev));
}

void MainWindow::on_textEdit_2_textChanged()
{
    QString plainText = ui->textEdit_2->toPlainText();
    //qDebug(plainText.toLatin1());
}
