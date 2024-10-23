#include <stdarg.h>
#include <stdlib.h>
#include "test.h"

/* Test status state. */
static unsigned test_count = 0;
static test_status_t test_counts[test_status_count] = { 0, 0, 0 };
static test_status_t test_status = test_status_pass;
static const char *test_name = "";
static unsigned test_errs = 0;

static const char *test_status_string(test_status_t current_status)
{
	switch (current_status) {
	case test_status_pass:
		return "pass";
	case test_status_skip:
		return "skip";
	case test_status_fail:
		return "fail";
	default:
		return "not define";
	}
}

void p_test_init(const char *name)
{
	test_count++;
	test_status = test_status_pass;
	test_name = name;
	test_errs = 0;
}

void p_test_fini(void)
{
	test_counts[test_status]++;
	printf("%s fail %u times\n", test_name, test_errs);
}

void p_test_fail(bool may_abort, const char *prefix, const char *message)
{
	bool colored = test_counts[test_status_fail] != 0 && isatty(STDERR_FILENO);
	const char *color_start = colored ? "\033[1;31m" : "";
	const char *color_end = colored ? "\033[0m" : "";
	printf("%s%s%s\n%s", color_start, prefix, message, color_end);
	test_status = test_status_fail;
	if (may_abort) {
		abort();
	} else {
		test_errs++;
	}
}

static test_status_t p_test_impl(test_t *t, va_list ap)
{
	test_status_t ret;

	ret = test_status_pass;
	for (; t != NULL; t = va_arg(ap, test_t *)) {
		t();
		if (test_status > ret) {
			ret = test_status;
		}
	}

	bool colored = test_counts[test_status_fail] != 0 && isatty(STDERR_FILENO);
	const char *color_start = colored ? "\033[1;31m" : "";
	const char *color_end = colored ? "\033[0m" : "";
	printf("%s--- %s: %u/%u, %s: %u/%u, %s: %u/%u ---\n%s", color_start, test_status_string(test_status_pass), test_counts[test_status_pass],
	       test_count, test_status_string(test_status_skip), test_counts[test_status_skip], test_count, test_status_string(test_status_fail),
	       test_counts[test_status_fail], test_count, color_end);

	return ret;
}

test_status_t p_test(test_t *t, ...)
{
	test_status_t ret;
	va_list ap;

	ret = test_status_pass;
	va_start(ap, t);
	ret = p_test_impl(t, ap);
	va_end(ap);

	return ret;
}
