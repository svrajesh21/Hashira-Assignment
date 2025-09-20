// Reads JSON with n, k, and root entries { "base": "...", "value": "..." }.
// Parses first k roots, converts to int64, builds monic polynomial with those roots,
// and prints coefficients from highest degree to constant term.
// Polynomial building uses iterative convolution: start with [1], then for each root r, multiply by (x - r).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

typedef struct {
    int base;
    char value[2048];
    int64_t as_int;
    int present;
} Root;

static int64_t parse_in_base(const char *s, int base, int *ok) {
    // parse s as unsigned then apply sign if leading '-'
    *ok = 1;
    int neg = 0;
    const char *p = s;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '+') { p++; }
    else if (*p == '-') { neg = 1; p++; }
    if (*p == 0) { *ok = 0; return 0; }
    int64_t acc = 0;
    while (*p) {
        char c = *p;
        if (c == '_' || isspace((unsigned char)c)) { p++; continue; }
        int v = -1;
        if (c >= '0' && c <= '9') v = c - '0';
        else if (c >= 'a' && c <= 'f') v = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') v = 10 + (c - 'A');
        else { *ok = 0; return 0; }
        if (v >= base) { *ok = 0; return 0; }
        // simple overflow check
        if (acc > (INT64_MAX - v) / base) { *ok = 0; return 0; }
        acc = acc * base + v;
        p++;
    }
    if (neg) {
        if (acc > (uint64_t)INT64_MAX + 1ULL) { *ok = 0; return 0; }
        return -acc;
    }
    return acc;
}

static int read_all_stdin(char **out, size_t *outlen) {
    size_t cap = 1 << 16;
    size_t len = 0;
    char *buf = (char*)malloc(cap);
    if (!buf) return 0;
    int c;
    while ((c = fgetc(stdin)) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char *nb = (char*)realloc(buf, cap);
            if (!nb) { free(buf); return 0; }
            buf = nb;
        }
        buf[len++] = (char)c;
    }
    buf[len] = 0;
    *out = buf;
    *outlen = len;
    return 1;
}

// Very simple JSON-ish extraction tailored to the provided schema.
static int extract_int_key(const char *json, const char *key, int *val_out) {
    // finds "key": <number>
    char pat[128];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(json, pat);
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    int sign = 1;
    if (*p == '-') { sign = -1; p++; }
    if (!isdigit((unsigned char)*p)) return 0;
    long v = 0;
    while (isdigit((unsigned char)*p)) {
        v = v * 10 + (*p - '0');
        p++;
    }
    *val_out = (int)(v * sign);
    return 1;
}

static int extract_block(const char *json, const char *idx_str, int *base_out, char *value_out, size_t value_cap) {
    // looks for "<idx_str>": { "base": "...", "value": "..." }
    char pat[64];
    snprintf(pat, sizeof(pat), "\"%s\"", idx_str);
    const char *p = strstr(json, pat);
    if (!p) return 0;
    p = strchr(p, '{');
    if (!p) return 0;
    const char *q = strchr(p, '}');
    if (!q) return 0;

    // Extract base
    int base = -1;
    {
        const char *b = strstr(p, "\"base\"");
        if (!b || b > q) return 0;
        b = strchr(b, ':');
        if (!b) return 0;
        b++;
        while (*b && isspace((unsigned char)*b)) b++;
        // base may be quoted or plain number
        if (*b == '"') {
            b++;
            char tmp[32]; size_t ti = 0;
            while (*b && *b != '"' && ti + 1 < sizeof(tmp)) { tmp[ti++] = *b; b++; }
            tmp[ti] = 0;
            if (*b != '"') return 0;
            char *endp = NULL;
            long bv = strtol(tmp, &endp, 10);
            if (endp == tmp) return 0;
            base = (int)bv;
        } else {
            if (!isdigit((unsigned char)*b)) return 0;
            long bv = 0;
            while (isdigit((unsigned char)*b)) { bv = bv * 10 + (*b - '0'); b++; }
            base = (int)bv;
        }
    }

    // Extract value string
    {
        const char *v = strstr(p, "\"value\"");
        if (!v || v > q) return 0;
        v = strchr(v, ':');
        if (!v) return 0;
        v++;
        while (*v && isspace((unsigned char)*v)) v++;
        if (*v != '"') return 0;
        v++;
        size_t vi = 0;
        while (*v && *v != '"' && vi + 1 < value_cap) { value_out[vi++] = *v; v++; }
        value_out[vi] = 0;
        if (*v != '"') return 0;
    }

    *base_out = base;
    return 1;
}

int main(void) {
    char *json = NULL;
    size_t jlen = 0;
    if (!read_all_stdin(&json, &jlen)) {
        fprintf(stderr, "Failed to read input\n");
        return 1;
    }

    int n = 0, k = 0;
    if (!extract_int_key(json, "n", &n) || !extract_int_key(json, "k", &k)) {
        fprintf(stderr, "Failed to extract n or k\n");
        free(json);
        return 1;
    }
    if (k <= 0 || n <= 0) {
        fprintf(stderr, "Invalid n or k\n");
        free(json);
        return 1;
    }

    Root *roots = (Root*)calloc((size_t)n, sizeof(Root));
    if (!roots) { free(json); return 1; }

    int found = 0;
    for (int i = 0; i < n; i++) {
        char idx[32];
        snprintf(idx, sizeof(idx), "%d", i + 1);
        int base = 0;
        char val[2048];
        if (extract_block(json, idx, &base, val, sizeof(val))) {
            roots[found].base = base;
            strncpy(roots[found].value, val, sizeof(roots[found].value) - 1);
            roots[found].value[sizeof(roots[found].value) - 1] = 0;
            int ok = 0;
            int64_t x = parse_in_base(val, base, &ok);
            if (!ok) {
                fprintf(stderr, "Failed to parse value at index %s\n", idx);
                free(roots);
                free(json);
                return 1;
            }
            roots[found].as_int = x;
            roots[found].present = 1;
            found++;
        }
    }

    if (found < k) {
        fprintf(stderr, "Not enough roots: found %d, need %d\n", found, k);
        free(roots);
        free(json);
        return 1;
    }

    // Select first k roots
    int64_t *r = (int64_t*)malloc((size_t)k * sizeof(int64_t));
    if (!r) { free(roots); free(json); return 1; }
    for (int i = 0; i < k; i++) r[i] = roots[i].as_int;

    // Build polynomial coefficients: start with P(x) = 1
    // After processing all k roots, P(x) = x^k + c1 x^(k-1) + ... + ck
    int64_t *coef = (int64_t*)calloc((size_t)(k + 1), sizeof(int64_t));
    if (!coef) { free(r); free(roots); free(json); return 1; }
    coef[0] = 1; // degree 0 polynomial = 1
    int deg = 0;

    for (int i = 0; i < k; i++) {
        int64_t ri = r[i];
        // new coefficients q for (current)* (x - ri)
        int64_t *q = (int64_t*)calloc((size_t)(deg + 2), sizeof(int64_t));
        if (!q) { free(coef); free(r); free(roots); free(json); return 1; }
        // x * current
        for (int j = 0; j <= deg; j++) q[j] += 0; // no-op for clarity
        for (int j = 0; j <= deg; j++) {
            // q[j] corresponds to coef of x^j in new after multiplication?
            // We keep coef as ascending powers: coef[j] = coeff of x^j.
            // Multiply: (sum a_j x^j) * (x - r) = sum a_j x^{j+1} - r a_j x^{j}
            // So:
            q[j + 1] += coef[j];       // from x * a_j x^j
            q[j]     -= ri * coef[j];  // from -r * a_j x^j
        }
        free(coef);
        coef = q;
        deg += 1;
    }

    // Print chosen roots
    printf("k\n");
    printf("%d\n", k);
    printf("roots_decimal_first_k\n");
    for (int i = 0; i < k; i++) {
        printf("%lld\n", (long long)r[i]);
    }

    // Print coefficients highest degree to constant:
    // coef array is ascending powers: coef[deg] ... coef[0]
    // For monic, coef[deg] should be 1.
    printf("degree\n");
    printf("%d\n", deg);
    printf("coefficients_high_to_low\n");
    for (int j = deg; j >= 0; j--) {
        printf("%lld\n", (long long)coef[j]);
    }

    free(coef);
    free(r);
    free(roots);
    free(json);
    return 0;
}