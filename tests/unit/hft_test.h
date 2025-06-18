// hft_test.h
#pragma once
#include <iostream>
#include <vector>
#include <cassert>
#include <chrono>

#define TEST_CASE(name) \
    void test_##name(); \
    struct test_##name##_registrar { \
        test_##name##_registrar() { \
            TestRunner::register_test(#name, test_##name); \
        } \
    } test_##name##_instance; \
    void test_##name()

#define EXPECT_TRUE(x) \
if (!(x)) std::cerr << "EXPECT_TRUE failed: " #x " at " << __FILE__ << ":" << __LINE__ << "\n";

#define EXPECT_EQ(a, b) EXPECT_TRUE((a) == (b))
#define EXPECT_GT(a, b) EXPECT_TRUE((a) > (b))
#define EXPECT_LT(a, b) EXPECT_TRUE((a) < (b))  // <-- Add this

#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            throw std::runtime_error("Assertion failed: " #cond); \
        } \
    } while(0)

#define EXPECT_THROW(expr, ex_type) \
    do { \
        bool threw = false; \
        try { expr; } \
        catch (const ex_type&) { threw = true; } \
        catch (...) {} \
        if (!threw) { \
            throw std::runtime_error("Expected exception: " #ex_type); \
        } \
    } while(0)
    
class TestRunner {
public:
    using TestFunc = void(*)();
    
    static void register_test(const char* name, TestFunc func) {
        tests().emplace_back(name, func);
    }
    
    static int run_all() {
        int passed = 0;
        for (const auto& [name, func] : tests()) {
            std::cout << "[ RUN      ] " << name << std::endl;
            try {
                func();
                std::cout << "[       OK ] " << name << std::endl;
                ++passed;
            } catch (const std::exception& e) {
                std::cout << "[  FAILED  ] " << name << " - " << e.what() << std::endl;
            }
        }
        std::cout << passed << "/" << tests().size() << " tests passed\n";
        return tests().size() - passed;
    }

private:
    static std::vector<std::pair<const char*, TestFunc>>& tests() {
        static std::vector<std::pair<const char*, TestFunc>> instance;
        return instance;
    }
};

