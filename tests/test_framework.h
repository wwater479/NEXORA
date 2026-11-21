#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <stdexcept>
#include <cmath>

namespace test_framework {

struct TestCase {
    std::string suite_name;
    std::string test_name;
    std::function<void()> func;
};

class TestRunner {
public:
    static TestRunner& Instance() {
        static TestRunner instance;
        return instance;
    }

    void RegisterTest(const std::string& suite, const std::string& name, std::function<void()> func) {
        tests_.push_back({suite, name, func});
    }

    int RunAll() {
        int passed = 0;
        int failed = 0;
        auto start_total = std::chrono::high_resolution_clock::now();

        std::cout << "========================================================\n";
        std::cout << "        NEXORA BROWSER AUTOMATED TEST SUITE             \n";
        std::cout << "========================================================\n\n";

        std::string current_suite = "";
        for (const auto& test : tests_) {
            if (test.suite_name != current_suite) {
                current_suite = test.suite_name;
                std::cout << "\n[SUITE] " << current_suite << "\n";
                std::cout << "--------------------------------------------------------\n";
            }

            auto start = std::chrono::high_resolution_clock::now();
            try {
                test.func();
                auto end = std::chrono::high_resolution_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
                std::cout << "  [PASS] " << test.test_name << " (" << ms << " ms)\n";
                passed++;
            } catch (const std::exception& e) {
                auto end = std::chrono::high_resolution_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
                std::cout << "  [FAIL] " << test.test_name << " (" << ms << " ms) -> " << e.what() << "\n";
                failed++;
            } catch (...) {
                std::cout << "  [FAIL] " << test.test_name << " -> Unknown exception\n";
                failed++;
            }
        }

        auto end_total = std::chrono::high_resolution_clock::now();
        auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_total - start_total).count();

        std::cout << "\n========================================================\n";
        std::cout << "TEST RESULTS: " << passed << " Passed, " << failed << " Failed, Total Time: " << total_ms << " ms\n";
        std::cout << "========================================================\n";

        return failed == 0 ? 0 : 1;
    }

private:
    std::vector<TestCase> tests_;
};

struct TestRegistrar {
    TestRegistrar(const std::string& suite, const std::string& name, std::function<void()> func) {
        TestRunner::Instance().RegisterTest(suite, name, func);
    }
};

} // namespace test_framework

#define TEST_CASE(suite, name) \
    static void test_##suite##_##name(); \
    static test_framework::TestRegistrar reg_##suite##_##name(#suite, #name, test_##suite##_##name); \
    static void test_##suite##_##name()

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            throw std::runtime_error(std::string("Assertion failed: ") + #expr + " at line " + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_FALSE(expr) \
    do { \
        if (expr) { \
            throw std::runtime_error(std::string("Assertion failed: NOT ") + #expr + " at line " + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            throw std::runtime_error(std::string("Assertion failed: ") + #a + " == " + #b + " at line " + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            throw std::runtime_error(std::string("Assertion failed: ") + #a + " != " + #b + " at line " + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_NEAR(a, b, eps) \
    do { \
        if (std::abs((a) - (b)) > (eps)) { \
            throw std::runtime_error(std::string("Assertion failed: |") + #a + " - " + #b + "| <= " + #eps + " at line " + std::to_string(__LINE__)); \
        } \
    } while (0)
