#include "test_framework.h"
#include "../src/storage/history_store.h"
#include "../src/storage/bookmark_store.h"
#include "../src/storage/session_store.h"
#include "../src/storage/download_store.h"
#include "../src/util/path_util.h"

using namespace nexora::storage;
using namespace nexora::util;

TEST_CASE(Storage, HistoryStoreOperationsAndSearch) {
    std::string test_path = "./test_history.json";
    PathUtil::DeleteFile(test_path);

    HistoryStore store(test_path);
    store.AddVisit("https://github.com/wwater479/NEXORA", "NEXORA Repository");
    store.AddVisit("https://google.com", "Google Search");
    store.AddVisit("https://wikipedia.org", "Wikipedia Encyclopedia");

    ASSERT_EQ(store.GetTotalCount(), 3);

    auto search_results = store.Search("NEXORA");
    ASSERT_EQ(search_results.size(), 1);
    ASSERT_EQ(search_results[0].title, "NEXORA Repository");

    ASSERT_TRUE(store.Save());

    HistoryStore loaded_store(test_path);
    ASSERT_TRUE(loaded_store.Load());
    ASSERT_EQ(loaded_store.GetTotalCount(), 3);

    ASSERT_TRUE(loaded_store.RemoveUrl("https://google.com"));
    ASSERT_EQ(loaded_store.GetTotalCount(), 2);

    PathUtil::DeleteFile(test_path);
}

TEST_CASE(Storage, BookmarkStoreHierarchyAndCRUD) {
    std::string test_path = "./test_bookmarks.json";
    PathUtil::DeleteFile(test_path);

    BookmarkStore store(test_path);
    auto folder = store.AddFolder(store.GetBookmarkBar()->id, "Dev Tools");
    ASSERT_NE(folder, nullptr);

    auto bm1 = store.AddBookmark(folder->id, "GitHub", "https://github.com");
    auto bm2 = store.AddBookmark(store.GetBookmarkBar()->id, "Google", "https://google.com");

    ASSERT_TRUE(store.IsBookmarked("https://github.com"));
    ASSERT_TRUE(store.IsBookmarked("https://google.com"));
    ASSERT_FALSE(store.IsBookmarked("https://nonexistent.org"));

    ASSERT_TRUE(store.UpdateNode(bm1->id, "GitHub Official"));
    ASSERT_EQ(store.FindById(bm1->id)->title, "GitHub Official");

    ASSERT_TRUE(store.Save());

    BookmarkStore loaded_store(test_path);
    ASSERT_TRUE(loaded_store.Load());
    ASSERT_TRUE(loaded_store.IsBookmarked("https://github.com"));

    ASSERT_TRUE(loaded_store.RemoveNode(bm2->id));
    ASSERT_FALSE(loaded_store.IsBookmarked("https://google.com"));

    PathUtil::DeleteFile(test_path);
}

TEST_CASE(Storage, SessionStorePersistenceAndCrashRecovery) {
    std::string test_path = "./test_session.json";
    PathUtil::DeleteFile(test_path);

    SessionStore store(test_path);
    BrowserSessionData session;
    session.crashed = true;

    WindowSessionData win;
    win.window_id = 1;
    win.x = 100;
    win.y = 100;
    win.width = 1200;
    win.height = 800;

    TabSessionData tab1;
    tab1.id = 1;
    tab1.url = "https://github.com";
    tab1.title = "GitHub";
    tab1.lifecycle_state = "ACTIVE";
    win.tabs.push_back(tab1);

    TabSessionData tab2;
    tab2.id = 2;
    tab2.url = "https://reddit.com";
    tab2.title = "Reddit";
    tab2.lifecycle_state = "DISCARDED";
    win.tabs.push_back(tab2);

    session.windows.push_back(win);

    ASSERT_TRUE(store.SaveSession(session));
    ASSERT_TRUE(store.HasCrashSession());

    BrowserSessionData loaded_session;
    ASSERT_TRUE(store.LoadSession(loaded_session));
    ASSERT_TRUE(loaded_session.crashed);
    ASSERT_EQ(loaded_session.windows.size(), 1);
    ASSERT_EQ(loaded_session.windows[0].tabs.size(), 2);
    ASSERT_EQ(loaded_session.windows[0].tabs[1].lifecycle_state, "DISCARDED");

    store.ClearCrashFlag();
    ASSERT_FALSE(store.HasCrashSession());

    PathUtil::DeleteFile(test_path);
    PathUtil::DeleteFile(test_path + ".bak");
}

TEST_CASE(Storage, DownloadStoreProgressAndState) {
    std::string test_path = "./test_downloads.json";
    PathUtil::DeleteFile(test_path);

    DownloadStore store(test_path);
    int64_t d_id = store.AddDownload("https://example.com/installer.exe", "installer.exe", "C:\\Downloads\\installer.exe", 50ULL * 1024 * 1024);
    ASSERT_TRUE(d_id > 0);

    store.UpdateProgress(d_id, 25ULL * 1024 * 1024, 50ULL * 1024 * 1024, 5.2 * 1024 * 1024);
    auto item = store.GetById(d_id);
    ASSERT_EQ(item.received_bytes, 25ULL * 1024 * 1024);

    store.UpdateState(d_id, DownloadState::Completed);
    ASSERT_EQ(store.GetById(d_id).state, DownloadState::Completed);

    ASSERT_EQ(store.GetActive().size(), 0);

    ASSERT_TRUE(store.Save());

    DownloadStore loaded_store(test_path);
    ASSERT_TRUE(loaded_store.Load());
    ASSERT_EQ(loaded_store.GetAll().size(), 1);

    PathUtil::DeleteFile(test_path);
}
