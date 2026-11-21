#include "test_framework.h"

int main() {
    return test_framework::TestRunner::Instance().RunAll();
}
