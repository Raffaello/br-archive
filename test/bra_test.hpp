#pragma once

#include <source_location>
#include <iostream>
#include <format>
#include <string>
#include <map>
#include <functional>
#include <cstdlib>    //for system

#if defined(__unix__) || defined(__APPLE__)
#include <sys/wait.h>
#endif

#define PRINT_TEST_NAME std::cout << std::format("[TEST] Running Test: {}", std::source_location::current().function_name()) << std::endl

// #define TEST_FUNC(x) "test_" #x, test_##x
#define TEST_FUNC(x) #x, x

#define TEST_INTERNAL_(x) _##x##_test()

#define TEST(x)                   \
    int TEST_INTERNAL_(x);        \
    int x()                       \
    {                             \
        PRINT_TEST_NAME;          \
                                  \
        /* user code */           \
        return TEST_INTERNAL_(x); \
    }                             \
    int TEST_INTERNAL_(x)


#define ASSERT_TRUE(x)                                                                \
    {                                                                                 \
        if (!(x))                                                                     \
        {                                                                             \
            auto l = std::source_location::current();                                 \
            std::cerr << std::format("[TEST FAILED {}] Expected true: '{}'\n({}:{})", \
                                     l.function_name(),                               \
                                     #x,                                              \
                                     l.file_name(),                                   \
                                     l.line())                                        \
                      << std::endl;                                                   \
            return 1;                                                                 \
        }                                                                             \
    }

#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))

#define ASSERT_EQ(x, y)                                                        \
    {                                                                          \
        auto res_x = x;                                                        \
        auto res_y = y;                                                        \
        if ((res_x) != (res_y))                                                \
        {                                                                      \
            auto l = std::source_location::current();                          \
            std::cerr << std::format("[TEST] {} FAILED:\n", l.function_name()) \
                      << std::format("--> '{}' != '{}'\n", #x, #y)             \
                      << std::format("Expected eq: '{}'\n", res_x)             \
                      << std::format("             '{}'\n", res_y)             \
                      << std::format("({}:{})\n", l.file_name(), l.line())     \
                      << std::endl;                                            \
            return 1;                                                          \
        }                                                                      \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__unix__) || defined(__APPLE__)
constexpr int EXITSTATUS(int ret)
{
    ASSERT_TRUE(WIFEXITED(ret));
    return WEXITSTATUS(ret);
}
#else
constexpr int EXITSTATUS(int ret)
{
    return ret;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

int call_system(const std::string& sys_args)
{
    std::cout << std::format("[TEST] CALLING: {}", sys_args) << std::endl;
    const int rc  = system(sys_args.c_str());
    const int ret = EXITSTATUS(rc);
    return ret;
}

int test_main(int argc, char* argv[], const std::map<std::string, std::function<int()>>& test_suite)
{
    if (argc >= 2)
    {
        return test_suite.at(argv[1])();
    }
    else
    {
        int ret = 0;

        std::cout << "Executing all tests..." << std::endl;
        for (const auto& [k, v] : test_suite)
            ret += v();

        return ret;
    }
}
