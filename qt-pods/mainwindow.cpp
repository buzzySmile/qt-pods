#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>

#include <QDebug>

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _localPods = new PodsModel();
    _remotePods = new PodsModel();

    _localPodsProxyModel = new QSortFilterProxyModel();
    _localPodsProxyModel->setSourceModel(_localPods);
    _localPodsProxyModel->setFilterKeyColumn(-1);

    _remotePodsProxyModel = new QSortFilterProxyModel();
    _remotePodsProxyModel->setSourceModel(_remotePods);
    _remotePodsProxyModel->setFilterKeyColumn(-1);

    ui->tableViewLocal->setModel(_localPodsProxyModel);
    ui->tableViewRemote->setModel(_remotePodsProxyModel);

    loadSettings();
    refreshAvailablePods();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_toolButtonRepository_clicked() {
    QString directory = QFileDialog::getExistingDirectory(this, tr("Add git repository"));
    if(!directory.isEmpty()) {
        if(!_podManager.isValidRepository(directory)) {
            QMessageBox::warning(this,
                tr("Invalid repository"),
                tr("The directory you supplied does not appear to be the root of a valid git repository."));
            return;
        }

        if(!ui->comboBoxCurrentRepository->findText(directory)) {
            ui->comboBoxCurrentRepository->addItem(directory);
        }

        saveSettings();

        refreshLocalPods();
    }
}

void MainWindow::on_pushButtonRemoveRepository_clicked() {
    int confirmationResult = QMessageBox::warning(this,
        tr("Confirm removal"),
        tr("Are you sure you want to remove the repository \"%1\"?")
            .arg(ui->comboBoxCurrentRepository->currentText()), QMessageBox::No, QMessageBox::Yes);
    if(confirmationResult == QMessageBox::Yes) {
        ui->comboBoxCurrentRepository->removeItem(ui->comboBoxCurrentRepository->currentIndex());
        saveSettings();
    }
}

void MainWindow::on_comboBoxCurrentRepository_currentTextChanged(QString text) {
    Q_UNUSED(text);
    refreshLocalPods();
}

void MainWindow::on_lineEditSearchLocal_textChanged(QString text) {
    _localPodsProxyModel->setFilterWildcard(text);
}

void MainWindow::on_lineEditSearchRemote_textChanged(QString text) {
    _remotePodsProxyModel->setFilterWildcard(text);
}

void MainWindow::on_pushButtonRemoveLocalPods_clicked() {
    QModelIndexList modelIndices = ui->tableViewLocal->selectionModel()->selectedRows(0);

    if(modelIndices.count() == 0) {
        return;
    }

    int confirmationResult;
    if(modelIndices.count() == 1) {
        confirmationResult = QMessageBox::warning(this,
            tr("Confirm removal"),
            tr("Are you sure you want to remove \"%1\" from the repository?")
                .arg(modelIndices.at(0).data().toString()), QMessageBox::No, QMessageBox::Yes);
    } else {
        confirmationResult = QMessageBox::warning(this,
            tr("Confirm removal"),
            tr("Are you sure you want to remove \"%1\" pods from the repository?")
                .arg(modelIndices.count()), QMessageBox::No, QMessageBox::Yes);
    }

    if(confirmationResult == QMessageBox::Yes) {
        foreach(QModelIndex modelIndex, modelIndices) {
            _podManager.removePod(ui->comboBoxCurrentRepository->currentText(),
                                  _localPods->item(modelIndex.row(), 0)->text());
        }
        refreshLocalPods();
    }
}

void MainWindow::on_pushButtonUpdateLocalPods_clicked() {
    _podManager.updatePods(ui->comboBoxCurrentRepository->currentText());
}

void MainWindow::on_pushButtonRefreshLocalPods_clicked() {
    refreshLocalPods();
}

void MainWindow::on_pushButtonRefreshAvailablePods_clicked() {
    refreshAvailablePods();
}

void MainWindow::on_pushButtonInstallPods_clicked() {

}

void MainWindow::closeEvent(QCloseEvent *closeEvent) {
    saveSettings();
    QMainWindow::closeEvent(closeEvent);
}

void MainWindow::loadSettings() {
    QSettings settings("qt-pods", "qt-pods");

    QStringList repositories = settings.value("local-repositories").toStringList();
    ui->comboBoxCurrentRepository->addItems(repositories);
}

void MainWindow::saveSettings() {
    QSettings settings("qt-pods", "qt-pods");

    QStringList repositories;
    for(int i = 0; i < ui->comboBoxCurrentRepository->count(); i++) {
        repositories.append(ui->comboBoxCurrentRepository->itemText(i));
    }
    settings.setValue("local-repositories", repositories);
    settings.sync();
}

void MainWindow::refreshLocalPods() {
    QString repository = ui->comboBoxCurrentRepository->currentText();
    if(!_podManager.isValidRepository(repository)) {
        QMessageBox::warning(this,
            tr("Invalid repository"),
            tr("The directory you supplied does not appear to be the root of a valid git repository."));

        while(int index = ui->comboBoxCurrentRepository->findText(repository)) {
            ui->comboBoxCurrentRepository->removeItem(index);
        }
        return;
    }

    QList<PodManager::Pod> pods = _podManager.installedPods(repository);

    // Clear model
    _localPods->reset();
    foreach(PodManager::Pod pod, pods) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(pod.name));
        row.append(new QStandardItem(pod.url));

        foreach(QStandardItem *item, row) {
            item->setEditable(false);
        }

        _localPods->appendRow(row);
    }
}


void MainWindow::refreshAvailablePods() {
    QStringList sources;
    sources << "https://raw.githubusercontent.com/cybercatalyst/qt-pods-master/master/pods.json";

    QList<PodManager::Pod> pods = _podManager.availablePods(sources);

    // Clear model
    _remotePods->reset();
    foreach(PodManager::Pod pod, pods) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(pod.name));
        row.append(new QStandardItem(pod.url));

        foreach(QStandardItem *item, row) {
            item->setEditable(false);
        }

        _remotePods->appendRow(row);
    }
}

