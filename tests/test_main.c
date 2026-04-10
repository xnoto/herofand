#include <assert.h>

int herofand_test_config(void);
int herofand_test_policy(void);

int main(void) {
    assert(herofand_test_config() == 0);
    assert(herofand_test_policy() == 0);
    return 0;
}
