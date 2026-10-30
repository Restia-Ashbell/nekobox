#include "ui/JsonEditor.hpp"
#include "ui_JsonEditor.h"

#include <QJsonArray>
#include <QJsonParseError>

#include "common/Utils.hpp"

namespace {
    // Drops // line comments and /* */ block comments that sit outside string
    // literals, so commented JSON can be parsed by QJsonDocument.
    QString stripJsonComments(const QString &json) {
        QString out;
        out.reserve(json.size());
        bool in_string = false;
        for (qsizetype i = 0; i < json.size(); ++i) {
            const QChar c = json.at(i);
            if (in_string) {
                out.append(c);
                if (c == '\\' && i + 1 < json.size()) {
                    out.append(json.at(++i));
                } else if (c == '"') {
                    in_string = false;
                }
                continue;
            }
            if (c == '"') {
                in_string = true;
            } else if (c == '/' && i + 1 < json.size() && json.at(i + 1) == '/') {
                const auto end = json.indexOf('\n', i + 2);
                if (end < 0) break;
                i = end - 1; // the loop increment keeps the newline
                continue;
            } else if (c == '/' && i + 1 < json.size() && json.at(i + 1) == '*') {
                const auto end = json.indexOf("*/", i + 2);
                if (end < 0) break;
                i = end + 1;
                continue;
            }
            out.append(c);
        }
        return out;
    }

    // One [key, value] row per JSON entry; containers get their children as rows.
    QList<QStandardItem *> makeJsonRow(const QString &key, const QJsonValue &value) {
        auto *key_item = new QStandardItem(key);
        auto *value_item = new QStandardItem;
        if (value.isObject()) {
            const auto object = value.toObject();
            for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
                key_item->appendRow(makeJsonRow(it.key(), it.value()));
            }
        } else if (value.isArray()) {
            const auto array = value.toArray();
            for (qsizetype i = 0; i < array.size(); ++i) {
                key_item->appendRow(makeJsonRow(QString::number(i), array.at(i)));
            }
        } else if (value.isString()) {
            value_item->setText(value.toString());
        } else if (value.isDouble()) {
            value_item->setText(QString::number(value.toDouble(), 'g', 15));
        } else if (value.isBool()) {
            value_item->setText(value.toBool() ? QStringLiteral("true") : QStringLiteral("false"));
        } else {
            value_item->setText(QStringLiteral("null"));
        }
        return {key_item, value_item};
    }
} // namespace

JsonEditor::JsonEditor(const QString &json, QWidget *parent)
    : QDialog(parent), ui(new Ui::JsonEditor), tree_model(new QStandardItemModel(this)) {
    ui->setupUi(this);
    ui->jsonTree->setModel(tree_model);
    tree_model->setHorizontalHeaderLabels({QStringLiteral("key"), QStringLiteral("value")});

    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError) {
        original = document;
        ui->jsonEditor->setPlainText(document.toJson(QJsonDocument::Indented));
        showStructure(document);
    } else {
        MessageBoxWarning(tr("Json Contains Syntax Errors"), tr("Original Json may contain syntax errors. Json tree is disabled."));
        ui->jsonEditor->setPlainText(json);
        setErrorState(true, error.errorString());
    }

    connect(ui->jsonEditor, &QTextEdit::textChanged, this, &JsonEditor::validate);
    connect(ui->formatJsonBtn, &QPushButton::clicked, this, &JsonEditor::formatJson);
    connect(ui->removeCommentsBtn, &QPushButton::clicked, this, &JsonEditor::removeComments);
}

JsonEditor::~JsonEditor() {
    delete ui;
}

QJsonObject JsonEditor::OpenEditor() {
    while (exec() == QDialog::Accepted) {
        const auto text = ui->jsonEditor->toPlainText();
        if (text.trimmed().isEmpty()) return {};
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(text.toUtf8(), &error);
        if (error.error == QJsonParseError::NoError) return document.object();
        MessageBoxWarning(tr("Json Contains Syntax Errors"), tr("You must correct these errors before continuing."));
    }
    return original.object();
}

void JsonEditor::validate() {
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(ui->jsonEditor->toPlainText().toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        setErrorState(true, error.errorString());
        return;
    }
    setErrorState(false);
    showStructure(document);
}

void JsonEditor::formatJson() {
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(ui->jsonEditor->toPlainText().toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        MessageBoxWarning(tr("Syntax Errors"), tr("Please fix the JSON errors or remove the comments before continue"));
        return;
    }
    ui->jsonEditor->setPlainText(document.toJson(QJsonDocument::Indented));
}

void JsonEditor::removeComments() {
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(stripJsonComments(ui->jsonEditor->toPlainText()).toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        MessageBoxWarning(tr("Syntax Errors"), tr("Please fix the JSON errors or remove the comments before continue"));
        return;
    }
    ui->jsonEditor->setPlainText(document.toJson(QJsonDocument::Indented));
}

void JsonEditor::showStructure(const QJsonDocument &doc) {
    const auto compact = doc.toJson(QJsonDocument::Compact);
    if (compact == shown_structure) return; // only whitespace changed
    shown_structure = compact;

    tree_model->clear();
    tree_model->setHorizontalHeaderLabels({QStringLiteral("key"), QStringLiteral("value")});
    const auto root = doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
    tree_model->invisibleRootItem()->appendRow(makeJsonRow(QStringLiteral("root"), root));
    ui->jsonTree->expandAll();
    ui->jsonTree->resizeColumnToContents(0);
}

void JsonEditor::setErrorState(bool error, const QString &status) {
    if (error_shown != error) {
        error_shown = error;
        QPalette palette;
        if (error) palette.setColor(QPalette::Text, Qt::red);
        ui->jsonEditor->setPalette(palette);
    }
    if (ui->jsonValidateStatus->text() != status) {
        ui->jsonValidateStatus->setText(status);
    }
}
