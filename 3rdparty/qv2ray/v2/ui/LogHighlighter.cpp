#include "LogHighlighter.hpp"

#define REGEX_IPV6_ADDR \
    R"(\[\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:)))(%.+)?\s*\])"
#define REGEX_IPV4_ADDR \
    R"((\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5]))"
#define REGEX_PORT_NUMBER \
    R"((?:[0-9]|[1-9]\d{1,3}|[1-5]\d{4}|6[0-5]{2}[0-3][0-5])+)"
#define REGEX_DOMAIN_ADDR \
    R"((?:[a-zA-Z0-9](?:[a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?\.)+[a-zA-Z]{2,6}/?)"

namespace Qv2ray::ui {
    SyntaxHighlighter::SyntaxHighlighter(bool darkMode, QTextDocument *parent)
        : QSyntaxHighlighter(parent), m_darkMode(darkMode) {
        HighlightingRule rule;

        {
            // IP:port, [IPv6]:port, host:port
            rule.pattern = QRegularExpression(REGEX_IPV4_ADDR ":" REGEX_PORT_NUMBER);
            rule.pattern.setPatternOptions(QRegularExpression::ExtendedPatternSyntaxOption);
            rule.format = &ipHostFormat;
            highlightingRules.append(rule);
            //
            rule.pattern = QRegularExpression(REGEX_IPV6_ADDR ":" REGEX_PORT_NUMBER);
            rule.pattern.setPatternOptions(QRegularExpression::ExtendedPatternSyntaxOption);
            rule.format = &ipHostFormat;
            highlightingRules.append(rule);
            //
            rule.pattern = QRegularExpression(REGEX_DOMAIN_ADDR ":" REGEX_PORT_NUMBER);
            rule.pattern.setPatternOptions(QRegularExpression::PatternOption::ExtendedPatternSyntaxOption);
            rule.format = &ipHostFormat;
            highlightingRules.append(rule);
        }

        rule.pattern = QRegularExpression(">>>>+");
        rule.format = &warningFormat;
        highlightingRules.append(rule);
        rule.pattern = QRegularExpression("<<<<+");
        rule.format = &warningFormat;
        highlightingRules.append(rule);

        applyColors();
    }

    void SyntaxHighlighter::setDarkMode(bool darkMode) {
        if (m_darkMode == darkMode) return;
        m_darkMode = darkMode;
        applyColors();
        rehighlight();
    }

    void SyntaxHighlighter::applyColors() {
        // sing-box palette (log/format.go): TRACE/DEBUG white, INFO cyan, WARN yellow,
        // ERROR/FATAL/PANIC red. Bold like aurora's bright level variants.
        if (m_darkMode) {
            traceDebugFormat.setForeground(Qt::white);
            infoFormat.setForeground(Qt::cyan);
            warnFormat.setForeground(Qt::yellow);
            errorFormat.setForeground(Qt::red);
            ipHostFormat.setForeground(Qt::yellow);
            warningFormat.setForeground(QColor(255, 160, 15));
        } else {
            traceDebugFormat.setForeground(QColor(110, 110, 110));
            infoFormat.setForeground(QColor(0, 128, 128));
            warnFormat.setForeground(QColor(178, 120, 0));
            errorFormat.setForeground(QColor(192, 0, 0));
            ipHostFormat.setForeground(QColor(30, 144, 255));
            warningFormat.setForeground(QColor(215, 106, 0));
        }
        for (auto *fmt: {&traceDebugFormat, &infoFormat, &warnFormat, &errorFormat}) {
            fmt->setFontWeight(QFont::Bold);
        }
        warningFormat.setFontWeight(QFont::Bold);
    }

    QColor SyntaxHighlighter::colorForId(quint32 id, bool dark) {
        // Same algorithm as sing-box log/format.go: id % 215 mapped onto the
        // 216-color cube; flip to the far side of the cube when it would clash
        // with the background (dark: too dark, light: too bright).
        const auto rgb = [](int row, int column) {
            return QColor(row * 51, column / 6 * 51, column % 6 * 51);
        };
        const auto luma = [](const QColor &c) {
            return 0.2126 * c.red() + 0.7152 * c.green() + 0.0722 * c.blue();
        };
        int color = static_cast<int>(id % 215);
        int row = color / 36;
        int column = color % 36;
        QColor c = rgb(row, column);
        if ((dark && luma(c) < 60) || (!dark && luma(c) > 128)) {
            c = rgb(5 - row, 35 - column);
        }
        return c;
    }

    void SyntaxHighlighter::highlightBlock(const QString &text) {
        // 1. Level token -> sing-box level colors. Case-insensitive so lowercase
        // logs from other cores colorize too; lookarounds prevent partial matches.
        static const QRegularExpression levelRe(
            R"((?<![A-Za-z])(TRACE|DEBUG|INFO|WARN|ERROR|FATAL|PANIC)(?![A-Za-z]))",
            QRegularExpression::CaseInsensitiveOption);
        auto it = levelRe.globalMatch(text);
        while (it.hasNext()) {
            const auto match = it.next();
            const QString level = match.captured(1);
            QTextCharFormat fmt;
            if (level.compare("INFO", Qt::CaseInsensitive) == 0) {
                fmt = infoFormat;
            } else if (level.compare("WARN", Qt::CaseInsensitive) == 0) {
                fmt = warnFormat;
            } else if (level.compare("ERROR", Qt::CaseInsensitive) == 0 ||
                       level.compare("FATAL", Qt::CaseInsensitive) == 0 ||
                       level.compare("PANIC", Qt::CaseInsensitive) == 0) {
                fmt = errorFormat;
            } else {
                fmt = traceDebugFormat;
            }
            setFormat(match.capturedStart(1), match.capturedLength(1), fmt);
        }

        // 2. Per-connection [id duration] prefix, e.g. [926152561 1m2s] -> per-id
        // cube color (only the id, like sing-box). The one-field [0042] elapsed
        // counter is left plain.
        static const QRegularExpression idRe(
            R"(\[(\d+) ((?:\d+)ms|(?:\d+(?:\.\d+)?)s|(?:\d+)m(?:\d+)s)\])");
        it = idRe.globalMatch(text);
        while (it.hasNext()) {
            const auto match = it.next();
            QTextCharFormat fmt;
            fmt.setForeground(colorForId(match.captured(1).toUInt(), m_darkMode));
            setFormat(match.capturedStart(1), match.capturedLength(1), fmt);
        }

        // 3. GUI extras: addresses, banners.
        for (const HighlightingRule &rule: highlightingRules) {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                const QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), *rule.format);
            }
        }

        setCurrentBlockState(0);
    }
} // namespace Qv2ray::ui