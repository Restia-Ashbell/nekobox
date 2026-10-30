#pragma once

#include <QColor>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

// Highlights sing-box style log lines: level tokens, per-connection ids and
// GUI-only extras (host:port addresses, banner separators).
class LogHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit LogHighlighter(bool darkMode, QTextDocument *document);

    void setDarkMode(bool darkMode);

protected:
    void highlightBlock(const QString &text) override;

private:
    void applyColors();

    bool m_darkMode;
    QTextCharFormat m_traceFormat;
    QTextCharFormat m_infoFormat;
    QTextCharFormat m_warnFormat;
    QTextCharFormat m_errorFormat;
    QTextCharFormat m_hostFormat;
    QTextCharFormat m_bannerFormat;
    QColor m_idColors[215];
};
