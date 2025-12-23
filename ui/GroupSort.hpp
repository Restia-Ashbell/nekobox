#pragma once

// implement in mainwindow
enum class GroupSortMethod {
    ByType,
    ByAddress,
    ByName,
    ByLatency,
    ById,
    Raw,
};

struct GroupSortAction {
    GroupSortMethod method = GroupSortMethod::Raw;
    bool save_sort = false;  // 保存到文件
    bool descending = false; // 默认升序，开这个就是降序
    bool scroll_to_started = false;
};
