#include "bra_test.hpp"
#include "lib_bra_defs.h"

#include <bra_fs.hpp>


using namespace std;

namespace fs = std::filesystem;

TEST(test_bra_fs_search_wildcard)
{
    std::set<fs::path> files;

#if defined(_WIN32)
    constexpr size_t exp_files = 2;    // bra.exe, bra.sfx
#else                                  // linux or whatever else
    constexpr size_t exp_files = 1;    // bra.sfx, bra.exe is just bra
#endif

    // No wildcard: function should return false and not modify the set
    ASSERT_FALSE(bra::fs::search_wildcard("bra", files));
    ASSERT_TRUE(files.empty());
    files.clear();

    // No wildcard: function should return false and not modify the set
    files.insert(fs::path("SENTINEL"));
    const auto before = files;
    ASSERT_FALSE(bra::fs::search_wildcard("bra", files));
    ASSERT_TRUE(files == before);
    files.clear();

    ASSERT_TRUE(bra::fs::search_wildcard("*", files));
    ASSERT_TRUE(files.size() != 0);
    files.clear();

    ASSERT_TRUE(bra::fs::search_wildcard("./*", files));
    ASSERT_TRUE(files.size() != 0);
    files.clear();

    ASSERT_TRUE(bra::fs::search_wildcard("bra.*", files));
    ASSERT_EQ(files.size(), exp_files);
    files.clear();

    ASSERT_TRUE(bra::fs::search_wildcard("./bra.*", files));
    ASSERT_EQ(files.size(), exp_files);
    files.clear();

    ASSERT_TRUE(bra::fs::search_wildcard("bra*", files));
    ASSERT_EQ(files.size(), 2U);
    files.clear();

    ASSERT_TRUE(bra::fs::search_wildcard("?ra.?fx", files));
    ASSERT_EQ(files.size(), 1U);
    ASSERT_EQ(files.begin()->string(), "bra.sfx");
    files.clear();

    ASSERT_TRUE(bra::fs::search_wildcard("dir?", files));
    ASSERT_EQ(files.size(), 0U);    // matches 'dir1' but as it is a dir without recursion is not added.
    // ASSERT_EQ(files.begin()->string(), "dir1");
    files.clear();

    return 0;
}

TEST(test_bra_fs_file_exists)
{
    ASSERT_TRUE(bra::fs::file_exists("test.txt"));
    ASSERT_FALSE(bra::fs::file_exists("test99.txt"));
    ASSERT_FALSE(bra::fs::file_exists("dir1"));

    return 0;
}

TEST(test_bra_fs_dir_exists)
{
    ASSERT_TRUE(bra::fs::dir_exists("dir1"));
    ASSERT_FALSE(bra::fs::dir_exists("dir99"));
    ASSERT_FALSE(bra::fs::dir_exists("test.txt"));

    return 0;
}

TEST(test_bra_fs_dir_make)
{
    const fs::path dir1 = "dir1/test";

    fs::remove(dir1);
    ASSERT_FALSE(bra::fs::dir_exists(dir1));
    ASSERT_TRUE(bra::fs::dir_make(dir1));
    ASSERT_TRUE(bra::fs::dir_exists(dir1));
    ASSERT_TRUE(bra::fs::dir_make(dir1));
    ASSERT_TRUE(bra::fs::dir_exists(dir1));

    fs::remove(dir1);
    ASSERT_FALSE(bra::fs::dir_exists(dir1));

    return 0;
}

int _test_bra_fs_try_sanitize_path(const fs::path& path, std::string_view s)
{
    auto p = path;

    cout << format("[TEST] try sanitize path: {}", path.string()) << endl;
    ASSERT_TRUE(bra::fs::try_sanitize(p));
    ASSERT_EQ(p.string(), s);

    return 0;
}

TEST(test_bra_fs_try_sanitize_path)
{
    fs::path p;

#if defined(_WIN32)
    p = "c:\\not_sane";
    ASSERT_FALSE(bra::fs::try_sanitize(p));
    p = "..\\not_sane";
    ASSERT_FALSE(bra::fs::try_sanitize(p));
#endif
    p = fs::current_path().parent_path();
    ASSERT_FALSE(bra::fs::try_sanitize(p));
    p = fs::current_path() / "..";
    ASSERT_FALSE(bra::fs::try_sanitize(p));

    ASSERT_EQ(_test_bra_fs_try_sanitize_path(fs::current_path(), "."), 0);
    ASSERT_EQ(_test_bra_fs_try_sanitize_path(fs::current_path() / "test.txt", "test.txt"), 0);
#ifdef __linux__
    ASSERT_EQ(_test_bra_fs_try_sanitize_path(fs::current_path() / "not_existing_dir" / ".." / "test.txt", ""), 1);
#else
    ASSERT_EQ(_test_bra_fs_try_sanitize_path(fs::current_path() / "not_existing_dir" / ".." / "test.txt", "test.txt"), 0);
#endif
    ASSERT_EQ(_test_bra_fs_try_sanitize_path("./wildcards/*", "wildcards/*"), 0);
    ASSERT_EQ(_test_bra_fs_try_sanitize_path("./*", "*"), 0);
    ASSERT_EQ(_test_bra_fs_try_sanitize_path("*", "*"), 0);
    ASSERT_EQ(_test_bra_fs_try_sanitize_path("./bra.*", "bra.*"), 0);
    ASSERT_EQ(_test_bra_fs_try_sanitize_path("bra.*", "bra.*"), 0);
    ASSERT_EQ(_test_bra_fs_try_sanitize_path("dir?", "dir?"), 0);
    ASSERT_EQ(_test_bra_fs_try_sanitize_path("./dir?", "dir?"), 0);

    return 0;
}

TEST(test_bra_fs_sfx_filename_adjust)
{
    auto exp = [](const string& s, const bool tmp) {
        return format("{}{}{}", s, BRA_FILE_EXT, tmp ? BRA_SFX_TMP_FILE_EXT : BRA_SFX_FILE_EXT);
    };

    bool tmp = false;
    ASSERT_EQ(bra::fs::filename_sfx_adjust("test", tmp).string(), exp("test", tmp));
    ASSERT_EQ(bra::fs::filename_sfx_adjust(string("test") + BRA_FILE_EXT, tmp).string(), exp("test", tmp));
    ASSERT_EQ(bra::fs::filename_sfx_adjust(string("test") + BRA_FILE_EXT + BRA_SFX_FILE_EXT, tmp).string(), exp("test", tmp));

    tmp = true;
    ASSERT_EQ(bra::fs::filename_sfx_adjust("test", tmp).string(), exp("test", tmp));
    ASSERT_EQ(bra::fs::filename_sfx_adjust(string("test") + BRA_FILE_EXT, tmp).string(), exp("test", tmp));
    ASSERT_EQ(bra::fs::filename_sfx_adjust(string("test") + BRA_FILE_EXT + BRA_SFX_FILE_EXT, tmp).string(), exp("test", tmp));

    return 0;
}

int main(int argc, char* argv[])
{
    return test_main(argc, argv, {
                                     {TEST_FUNC(test_bra_fs_search_wildcard)},
                                     {TEST_FUNC(test_bra_fs_file_exists)},
                                     {TEST_FUNC(test_bra_fs_dir_exists)},
                                     {TEST_FUNC(test_bra_fs_dir_make)},
                                     {TEST_FUNC(test_bra_fs_try_sanitize_path)},
                                     {TEST_FUNC(test_bra_fs_sfx_filename_adjust)},
                                 });
}
