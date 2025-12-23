#pragma once

class ThemeManager {
public:
    void ApplyTheme(const QString &theme);
};

extern ThemeManager *themeManager;
