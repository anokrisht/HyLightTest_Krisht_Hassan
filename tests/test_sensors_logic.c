/* tests/test_sensors_logic.c - verify ΔP computation logic */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

void compute_dp(const float press[6], const bool present[6], float dp_out[3]) {
    for (int i = 0; i < 3; ++i) {
        int a = i*2, b = i*2+1;
        if (present[a] && present[b]) dp_out[i] = press[b] - press[a];
        else dp_out[i] = 0.0f;
    }
}

int main(void) {
    float press[6] = {1000.0f, 1010.0f, 2000.0f, 1990.0f, 1500.0f, 1505.0f};
    bool present_all[6] = {true,true,true,true,true,true};
    float dp[3] = {0};
    compute_dp(press, present_all, dp);
    if (dp[0] != 10.0f || dp[1] != -10.0f || dp[2] != 5.0f) {
        printf("DP all present mismatch: %f %f %f\n", dp[0], dp[1], dp[2]);
        return 1;
    }

    bool missing_some[6] = {true,false,true,true,true,true};
    compute_dp(press, missing_some, dp);
    if (dp[0] != 0.0f) {
        printf("DP missing handling failed: dp0=%f\n", dp[0]);
        return 2;
    }

    printf("sensors_logic tests passed\n");
    return 0;
}
