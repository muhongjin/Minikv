#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>

#include "minikv.h"

namespace fs = std::filesystem;

#define ASSERT_TRUE(expression) \
    do { \
        if (!(expression)) { \
            std::cerr << "FAIL: " << __func__ << ": " << #expression \
                      << " at line " << __LINE__ << std::endl; \
            return false; \
        } \
    } while (false)

void ClearStorage()
{
    const char* logFiles[] = {
        "test_basic.log",
        "test_replay.log",
        "test_flush.log",
        "test_priority.log",
        "test_restart.log"
    };

    for (const char* logFile : logFiles) {
        std::remove(logFile);
    }

    for (int index = 1; index <= 32; ++index) {
        const std::string fileName = "sst_" + std::to_string(index) + ".sst";
        std::remove(fileName.c_str());
    }
}

bool ExpectValue(MiniKV& kv, const std::string& key, const std::string& expected)
{
    std::string actual;
    const Status status = kv.Get(key, &actual);
    return status.ok() && actual == expected;
}

bool ExpectMissing(MiniKV& kv, const std::string& key)
{
    std::string value;
    return kv.Get(key, &value).IsNotFound();
}

bool TestBasicCRUD()
{
    ClearStorage();
    {
        MiniKV kv("test_basic.log");

        ASSERT_TRUE(ExpectMissing(kv, "missing"));
        ASSERT_TRUE(kv.Set("name", "Alice").ok());
        ASSERT_TRUE(ExpectValue(kv, "name", "Alice"));
        ASSERT_TRUE(kv.Set("name", "Bob").ok());
        ASSERT_TRUE(ExpectValue(kv, "name", "Bob"));
        ASSERT_TRUE(kv.Delete("name").ok());
        ASSERT_TRUE(ExpectMissing(kv, "name"));
    }
    ClearStorage();
    return true;
}

bool TestWALReplay()
{
    ClearStorage();
    {
        MiniKV kv("test_replay.log");
        ASSERT_TRUE(kv.Set("name", "first").ok());
        ASSERT_TRUE(kv.Set("name", "second").ok());
        ASSERT_TRUE(kv.Set("role", "developer").ok());
        ASSERT_TRUE(kv.Delete("role").ok());
    }

    {
        MiniKV recovered("test_replay.log");
        ASSERT_TRUE(ExpectValue(recovered, "name", "second"));
        ASSERT_TRUE(ExpectMissing(recovered, "role"));
        ASSERT_TRUE(ExpectMissing(recovered, "unknown"));
    }
    ClearStorage();
    return true;
}

bool TestMemTableFlush()
{
    ClearStorage();
    {
        MiniKV kv("test_flush.log");
        ASSERT_TRUE(kv.Set("a", "1").ok());
        ASSERT_TRUE(kv.Set("b", "2").ok());
        ASSERT_TRUE(kv.Set("c", "3").ok());
        ASSERT_TRUE(kv.Set("d", "4").ok());
        ASSERT_TRUE(kv.Set("e", "5").ok());

        ASSERT_TRUE(ExpectValue(kv, "a", "1"));
        ASSERT_TRUE(ExpectValue(kv, "c", "3"));
        ASSERT_TRUE(ExpectValue(kv, "e", "5"));
        ASSERT_TRUE(ExpectMissing(kv, "missing"));
    }
    ClearStorage();
    return true;
}

bool TestLatestSSTWins()
{
    ClearStorage();
    {
        MiniKV kv("test_priority.log");
        ASSERT_TRUE(kv.Set("shared", "old").ok());
        ASSERT_TRUE(kv.Set("old_a", "1").ok());
        ASSERT_TRUE(kv.Set("old_b", "2").ok());
        ASSERT_TRUE(kv.Set("old_c", "3").ok());
        ASSERT_TRUE(kv.Set("old_d", "4").ok());

        ASSERT_TRUE(kv.Set("shared", "new").ok());
        ASSERT_TRUE(kv.Set("new_a", "1").ok());
        ASSERT_TRUE(kv.Set("new_b", "2").ok());
        ASSERT_TRUE(kv.Set("new_c", "3").ok());
        ASSERT_TRUE(kv.Set("new_d", "4").ok());

        ASSERT_TRUE(ExpectValue(kv, "shared", "new"));
        ASSERT_TRUE(ExpectValue(kv, "old_a", "1"));
        ASSERT_TRUE(ExpectValue(kv, "new_d", "4"));
    }
    ClearStorage();
    return true;
}

bool TestSSTSurvivesRestart()
{
    ClearStorage();
    {
        MiniKV kv("test_restart.log");
        ASSERT_TRUE(kv.Set("persist", "value").ok());
        ASSERT_TRUE(kv.Set("fill_a", "1").ok());
        ASSERT_TRUE(kv.Set("fill_b", "2").ok());
        ASSERT_TRUE(kv.Set("fill_c", "3").ok());
        ASSERT_TRUE(kv.Set("fill_d", "4").ok());
    }

    {
        MiniKV recovered("test_restart.log");
        ASSERT_TRUE(ExpectValue(recovered, "persist", "value"));
        ASSERT_TRUE(ExpectValue(recovered, "fill_d", "4"));
        ASSERT_TRUE(ExpectMissing(recovered, "missing"));
    }
    ClearStorage();
    return true;
}

bool RunTest(const char* name, bool (*test)())
{
    const bool passed = test();
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << name << "\n";
    return passed;
}

bool PrepareTestDirectory(fs::path& originalPath, fs::path& testPath)
{
    std::error_code error;
    originalPath = fs::current_path(error);
    if (error) {
        return false;
    }

    // 测试在独立目录中运行，避免清理真实运行数据。
    testPath = originalPath / "minikv_test_data";
    fs::remove_all(testPath, error);
    error.clear();
    fs::create_directories(testPath, error);
    if (error) {
        return false;
    }

    fs::current_path(testPath, error);
    return !error;
}

void CleanupTestDirectory(const fs::path& originalPath, const fs::path& testPath)
{
    std::error_code error;
    fs::current_path(originalPath, error);
    fs::remove_all(testPath, error);
}

int main()
{
    fs::path originalPath;
    fs::path testPath;
    if (!PrepareTestDirectory(originalPath, testPath)) {
        std::cerr << "FAIL: unable to prepare test directory\n";
        return 1;
    }

    bool allPassed = true;
    allPassed &= RunTest("TestBasicCRUD", TestBasicCRUD);
    allPassed &= RunTest("TestWALReplay", TestWALReplay);
    allPassed &= RunTest("TestMemTableFlush", TestMemTableFlush);
    allPassed &= RunTest("TestLatestSSTWins", TestLatestSSTWins);
    allPassed &= RunTest("TestSSTSurvivesRestart", TestSSTSurvivesRestart);

    CleanupTestDirectory(originalPath, testPath);
    std::cout << (allPassed ? "==== All tests passed ====\n" : "==== Some tests failed ====\n");
    return allPassed ? 0 : 1;
}
