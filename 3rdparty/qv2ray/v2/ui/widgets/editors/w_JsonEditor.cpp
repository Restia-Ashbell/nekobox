#include "w_JsonEditor.hpp"

JsonEditor::JsonEditor(const QString& jsonString, QWidget* parent) : QDialog(parent) {
    setupUi(this);
    //    QvMessageBusConnect(JsonEditor);
    //
    QJsonParseError err;
    original = QJsonDocument::fromJson(jsonString.toUtf8(), &err);
    final = original;
    auto formatJson = original.toJson(QJsonDocument::Indented);

    if (err.error == QJsonParseError::NoError) {
        model.loadJson(formatJson);
        jsonTree->setModel(&model);
    } else {
        QvMessageBoxWarn(this, tr("Json Contains Syntax Errors"),
                         tr("Original Json may contain syntax errors. Json tree is disabled."));
    }

    jsonEditor->setText(formatJson);
    jsonTree->expandAll();
    jsonTree->resizeColumnToContents(0);
}

// QvMessageBusSlotImpl(JsonEditor)
//         {
//                 switch (msg)
//                 {
//                     MBShowDefaultImpl;
//                     MBHideDefaultImpl;
//                     MBRetranslateDefaultImpl;
//                     case UPDATE_COLORSCHEME:
//                         break;
//                 }
//         }

QJsonObject JsonEditor::OpenEditor() {
    int resultCode = this->exec();
    QString jsonText = jsonEditor->toPlainText();

    while (resultCode == QDialog::Accepted && QJsonDocument::fromJson(jsonText.toUtf8()).isNull()) {
        if (jsonText.trimmed().isEmpty()) {
            resultCode = QDialog::Accepted;
            final = {};
            break;
        }
        QvMessageBoxWarn(this, tr("Json Contains Syntax Errors"),
                         tr("You must correct these errors before continuing."));
        resultCode = this->exec();
        jsonText = jsonEditor->toPlainText();
    }

    return (resultCode == QDialog::Accepted ? final : original).object();
}

JsonEditor::~JsonEditor() {
}

void JsonEditor::on_jsonEditor_textChanged() {
    const QString jsonText = jsonEditor->toPlainText();
    QJsonParseError err;
    QJsonDocument temp = QJsonDocument::fromJson(jsonText.toUtf8(), &err);

    if (err.error == QJsonParseError::NoError) {
        BLACK(jsonEditor);
        final = temp;
        model.loadJson(temp.toJson());
        jsonTree->expandAll();
        jsonTree->resizeColumnToContents(0);
        jsonValidateStatus->clear();
    } else {
        RED(jsonEditor);
        jsonValidateStatus->setText(err.errorString());
    }
}

void JsonEditor::on_formatJsonBtn_clicked() {
    const QString jsonText = jsonEditor->toPlainText();
    QJsonParseError err;
    auto formatJson = QJsonDocument::fromJson(jsonText.toUtf8(), &err).toJson(QJsonDocument::Indented);

    if (err.error == QJsonParseError::NoError) {
        BLACK(jsonEditor);
        jsonEditor->setPlainText(formatJson);
        model.loadJson(formatJson);
        jsonTree->setModel(&model);
        jsonTree->expandAll();
        jsonTree->resizeColumnToContents(0);
    } else {
        RED(jsonEditor);
        QvMessageBoxWarn(this, tr("Syntax Errors"),
                         tr("Please fix the JSON errors or remove the comments before continue"));
    }
}

void JsonEditor::on_removeCommentsBtn_clicked() {
    jsonEditor->setPlainText(JsonToString(JsonFromString(jsonEditor->toPlainText())));
}
