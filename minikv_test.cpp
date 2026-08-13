#include <cstdio>
#include <iostream>
#include <string>

#include "minikv.h"

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
    const char* logs[] = {
        "test_basic.log",
        "test_replay.log",
        "test_flush.log",
        "test_priority.log",
        "test_restart.log"
    };
    for (const char* log : logs) {
        std::remove(log);
    }
    for (int index = 1; index <= 8; ++index) {
        std::remove(("sst_" + std::to_string(index) + ".sst").c_str());
    }
}

bool ExpectValue(MiniKV& kv, const std::string& key, const std::string& expected)
{
    std::string actual;
    return kv.Get(key, &actual).ok() && actual == expected;
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
        ASSERT_TRUE(ExpectValue(kv, "e", "5"));
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

int main()
{
    bool allPassed = true;
    allPassed &= RunTest("TestBasicCRUD", TestBasicCRUD);
    allPassed &= RunTest("TestWALReplay", TestWALReplay);
    allPassed &= RunTest("TestMemTableFlush", TestMemTableFlush);
    allPassed &= RunTest("TestLatestSSTWins", TestLatestSSTWins);
    allPassed &= RunTest("TestSSTSurvivesRestart", TestSSTSurvivesRestart);
    std::cout << (allPassed ? "==== All tests passed ====\n"
                            : "==== Some tests failed ====\n");
    return allPassed ? 0 : 1;
}
