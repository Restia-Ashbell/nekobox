#include "ui/LogHighlighter.hpp"

#include <QFont>
#include <QRegularExpression>
#include <QTextDocument>

#define REGEX_IPV6_ADDR \
    R"(\[\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:)))(%.+)?\s*\])"
#define REGEX_IPV4_ADDR \
    R"((\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5]))"
#define REGEX_PORT_NUMBER \
    R"((?:[0-9]|[1-9]\d{1,3}|[1-5]\d{4}|6[0-5]{2}[0-3][0-5])+)"
#define REGEX_DOMAIN_ADDR \
    R"((?:[a-zA-Z0-9](?:[a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?\.)+[a-zA-Z]{2,6}/?)"

LogHighlighter::LogHighlighter(bool darkMode, QTextDocument *document) : QSyntaxHighlighter(document), m_darkMode(darkMode) {
    applyColors();
}

void LogHighlighter::setDarkMode(bool darkMode) {
    if (m_darkMode == darkMode) return;
    m_darkMode = darkMode;
    applyColors();
    rehighlight();
}

void LogHighlighter::applyColors() {
    // sing-box palette (log/format.go): TRACE/DEBUG white, INFO cyan, WARN yellow,
    // ERROR/FATAL/PANIC red. Bold like aurora's bright level variants.
    if (m_darkMode) {
        m_traceFormat.setForeground(Qt::white);
        m_infoFormat.setForeground(Qt::cyan);
        m_warnFormat.setForeground(Qt::yellow);
        m_errorFormat.setForeground(Qt::red);
        m_hostFormat.setForeground(Qt::yellow);
        m_bannerFormat.setForeground(QColor(255, 160, 15));
    } else {
        m_traceFormat.setForeground(QColor(110, 110, 110));
        m_infoFormat.setForeground(QColor(0, 128, 128));
        m_warnFormat.setForeground(QColor(178, 120, 0));
        m_errorFormat.setForeground(QColor(192, 0, 0));
        m_hostFormat.setForeground(QColor(30, 144, 255));
        m_bannerFormat.setForeground(QColor(215, 106, 0));
    }
    for (auto *format: {&m_traceFormat, &m_infoFormat, &m_warnFormat, &m_errorFormat, &m_bannerFormat}) {
        format->setFontWeight(QFont::Bold);
    }

    // Same algorithm as sing-box log/format.go: id % 215 mapped onto the 216-color
    // cube; flip to the far side of the cube when it would clash with the background
    // (dark: too dark, light: too bright). Precomputed once per color scheme.
    const auto rgb = [](int row, int column) {
        return QColor(row * 51, column / 6 * 51, column % 6 * 51);
    };
    const auto luma = [](const QColor &color) {
        return 0.2126 * color.red() + 0.7152 * color.green() + 0.0722 * color.blue();
    };
    for (int id = 0; id < 215; ++id) {
        const int row = id / 36;
        const int column = id % 36;
        QColor color = rgb(row, column);
        if ((m_darkMode && luma(color) < 60) || (!m_darkMode && luma(color) > 128)) {
            color = rgb(5 - row, 35 - column);
        }
        m_idColors[id] = color;
    }
}

void LogHighlighter::highlightBlock(const QString &text) {
    // 1. Level token -> sing-box level colors. Case-insensitive so lowercase logs
    // from other cores colorize too; lookarounds prevent partial matches.
    static const QRegularExpression levelRe(
        R"((?<![A-Za-z])(?:(?<trace>TRACE|DEBUG)|(?<warn>WARN)|(?<error>ERROR|FATAL|PANIC)|(?<info>INFO))(?![A-Za-z]))",
        QRegularExpression::CaseInsensitiveOption);
    auto matches = levelRe.globalMatch(text);
    while (matches.hasNext()) {
        const auto match = matches.next();
        const QTextCharFormat *format = &m_traceFormat;
        if (match.hasCaptured(QStringLiteral("info"))) {
            format = &m_infoFormat;
        } else if (match.hasCaptured(QStringLiteral("warn"))) {
            format = &m_warnFormat;
        } else if (match.hasCaptured(QStringLiteral("error"))) {
            format = &m_errorFormat;
        }
        setFormat(int(match.capturedStart()), int(match.capturedLength()), *format);
    }

    // 2. Per-connection [id duration] prefix, e.g. [926152561 1m2s] -> per-id cube
    // color (only the id, like sing-box). The one-field [0042] elapsed counter is
    // left plain.
    static const QRegularExpression idRe(R"(\[(\d+) ((?:\d+)ms|(?:\d+(?:\.\d+)?)s|(?:\d+)m(?:\d+)s)\])");
    matches = idRe.globalMatch(text);
    while (matches.hasNext()) {
        const auto match = matches.next();
        setFormat(int(match.capturedStart(1)), int(match.capturedLength(1)), m_idColors[match.capturedView(1).toUInt() % 215]);
    }

    // 3. GUI extras: addresses and banners.
    static const QRegularExpression hostRe(
        REGEX_IPV6_ADDR ":" REGEX_PORT_NUMBER "|" REGEX_IPV4_ADDR ":" REGEX_PORT_NUMBER "|" REGEX_DOMAIN_ADDR ":" REGEX_PORT_NUMBER);
    static const QRegularExpression bannerRe(R"([<>]{4,})");
    const auto applyAll = [this, &text](const QRegularExpression &pattern, const QTextCharFormat &format) {
        auto it = pattern.globalMatch(text);
        while (it.hasNext()) {
            const auto match = it.next();
            setFormat(int(match.capturedStart()), int(match.capturedLength()), format);
        }
    };
    applyAll(hostRe, m_hostFormat);
    applyAll(bannerRe, m_bannerFormat);
}
