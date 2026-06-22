#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct val_str {
	int value;
	const char *str;
};

static inline int value_from_string(const char *text, const struct val_str *table)
{
	if (!text || !table) {
		return 0;
	}
	for (int index = 0; table[index].str != NULL; ++index) {
		const char *candidate = table[index].str;
		const char *lhs = text;
		int matches = 1;
		while (*lhs != '\0' || *candidate != '\0') {
			if (*lhs != *candidate) {
				matches = 0;
				break;
			}
			++lhs;
			++candidate;
		}
		if (matches) {
			return table[index].value;
		}
	}
	return 0;
}

#ifdef __cplusplus
}
#endif
