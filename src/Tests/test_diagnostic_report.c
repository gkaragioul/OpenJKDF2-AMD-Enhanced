#include "General/DiagnosticReport.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "line %d: %s\n", __LINE__, #value); ++failures; } } while (0)

int main(void)
{
    DiagnosticReport report;
    char json[2048];
    CHECK(diag_report_build(&report, "0.9.9", "Windows", "11-26200"));
    CHECK(diag_report_to_json(&report, json, sizeof(json)));
    CHECK(strstr(json, "\"schema\":1") != NULL);
    CHECK(strstr(json, "\"renderer\":\"not_collected\"") != NULL);
    CHECK(strstr(json, "hostname") == NULL);
    CHECK(strstr(json, "username") == NULL);
    CHECK(strstr(json, "serial") == NULL);

    CHECK(diag_report_set_renderer(&report, "ATI Technologies Inc.", "AMD Radeon", "3.3", "4.60"));
    CHECK(diag_report_to_json(&report, json, sizeof(json)));
    CHECK(strstr(json, "\"renderer\":\"AMD Radeon\"") != NULL);
    CHECK(strstr(json, "\"api_version\":\"3.3\"") != NULL);
    return failures ? 1 : 0;
}
