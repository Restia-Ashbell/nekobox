#pragma once

#include <QDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardItemModel>

namespace Ui {
    class JsonEditor;
}

class JsonEditor : public QDialog {
    Q_OBJECT

public:
    explicit JsonEditor(const QString &json, QWidget *parent = nullptr);

    ~JsonEditor() override;

    QJsonObject OpenEditor();

private:
    void validate();
    void formatJson();
    void removeComments();
    void showStructure(const QJsonDocument &doc);
    void setErrorState(bool error, const QString &status = {});

    Ui::JsonEditor *ui;
    QStandardItemModel *tree_model;
    QJsonDocument original;
    QByteArray shown_structure;
    bool error_shown = false;
};
