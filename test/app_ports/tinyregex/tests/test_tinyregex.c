/* tiny-regex-c FakeCC port tests, adapted from upstream tests/test1.c.
 * The commented-out [^\\w][^-1-4] cases are inverted classes; upstream
 * marks INV_CHAR_CLASS as currently broken and leaves those rows disabled. */
package main;

import tinyregex;
import runtime;

typedef struct {
    int should_match;
    const char *pattern;
    const char *text;
    int matchlen;
} test_case;

static test_case tests[] = {
    { 1, "\\d",                       "5",                 1 },
    { 1, "\\w+",                      "hej",               3 },
    { 1, "\\s",                       "\t \n",             1 },
    { 0, "\\S",                       "\t \n",             0 },
    { 1, "[\\s]",                     "\t \n",             1 },
    { 0, "[\\S]",                     "\t \n",             0 },
    { 0, "\\D",                       "5",                 0 },
    { 0, "\\W+",                      "hej",               0 },
    { 1, "[0-9]+",                    "12345",             5 },
    { 1, "\\D",                       "hej",               1 },
    { 0, "\\d",                       "hej",               0 },
    { 1, "[^\\w]",                    "\\",                1 },
    { 1, "[\\W]",                     "\\",                1 },
    { 0, "[\\w]",                     "\\",                0 },
    { 1, "[^\\d]",                    "d",                 1 },
    { 0, "[\\d]",                     "d",                 0 },
    { 0, "[^\\D]",                    "d",                 0 },
    { 1, "[\\D]",                     "d",                 1 },
    { 1, "^.*\\\\.*$",                "c:\\Tools",         8 },
    { 1, "^.*\\\\.*$",                "c:\\Tools",         8 },
    { 1, ".?\\w+jsj$",                "%JxLLcVx8wxrjsj",  15 },
    { 1, ".?\\w+jsj$",                "=KbvUQjsj",         9 },
    { 1, ".?\\w+jsj$",                "^uDnoZjsj",         9 },
    { 1, ".?\\w+jsj$",                "UzZbjsj",           7 },
    { 1, ".?\\w+jsj$",                "\"wjsj",            5 },
    { 1, ".?\\w+jsj$",                "zLa_FTEjsj",       10 },
    { 1, ".?\\w+jsj$",                "\"mw3p8_Ojsj",     11 },
    { 1, "^[\\+-]*[\\d]+$",           "+27",               3 },
    { 1, "[abc]",                     "1c2",               1 },
    { 0, "[abc]",                     "1C2",               0 },
    { 1, "[1-5]+",                    "0123456789",        5 },
    { 1, "[.2]",                      "1C2",               1 },
    { 1, "a*$",                       "Xaa",               2 },
    { 1, "a*$",                       "Xaa",               2 },
    { 1, "[a-h]+",                    "abcdefghxxx",       8 },
    { 0, "[a-h]+",                    "ABCDEFGH",          0 },
    { 1, "[A-H]+",                    "ABCDEFGH",          8 },
    { 0, "[A-H]+",                    "abcdefgh",          0 },
    { 1, "[^\\s]+",                   "abc def",           3 },
    { 1, "[^fc]+",                    "abc def",           2 },
    { 1, "[^d\\sf]+",                 "abc def",           3 },
    { 1, "\n",                        "abc\ndef",          1 },
    { 1, "b.\\s*\n",                  "aa\r\nbb\r\ncc\r\n\r\n", 4 },
    { 1, ".*c",                       "abcabc",            6 },
    { 1, ".+c",                       "abcabc",            6 },
    { 1, "[b-z].*",                   "ab",                1 },
    { 1, "b[k-z]*",                   "ab",                1 },
    { 0, "[0-9]",                     "  - ",              0 },
    { 1, "[^0-9]",                    "  - ",              1 },
    { 1, "0|",                        "0|",                2 },
    { 0, "\\d\\d:\\d\\d:\\d\\d",      "0s:00:00",          0 },
    { 0, "\\d\\d:\\d\\d:\\d\\d",      "000:00",            0 },
    { 0, "\\d\\d:\\d\\d:\\d\\d",      "00:0000",           0 },
    { 0, "\\d\\d:\\d\\d:\\d\\d",      "100:0:00",          0 },
    { 0, "\\d\\d:\\d\\d:\\d\\d",      "00:100:00",         0 },
    { 0, "\\d\\d:\\d\\d:\\d\\d",      "0:00:100",          0 },
    { 1, "\\d\\d?:\\d\\d?:\\d\\d?",   "0:0:0",             5 },
    { 1, "\\d\\d?:\\d\\d?:\\d\\d?",   "0:00:0",            6 },
    { 1, "\\d\\d?:\\d\\d?:\\d\\d?",   "0:0:00",            5 },
    { 1, "\\d\\d?:\\d\\d?:\\d\\d?",   "00:0:0",            6 },
    { 1, "\\d\\d?:\\d\\d?:\\d\\d?",   "00:00:0",           7 },
    { 1, "\\d\\d?:\\d\\d?:\\d\\d?",   "00:0:00",           6 },
    { 1, "\\d\\d?:\\d\\d?:\\d\\d?",   "0:00:00",           6 },
    { 1, "\\d\\d?:\\d\\d?:\\d\\d?",   "00:00:00",          7 },
    { 1, "[Hh]ello [Ww]orld\\s*[!]?", "Hello world !",    12 },
    { 1, "[Hh]ello [Ww]orld\\s*[!]?", "hello world !",    12 },
    { 1, "[Hh]ello [Ww]orld\\s*[!]?", "Hello World !",    12 },
    { 1, "[Hh]ello [Ww]orld\\s*[!]?", "Hello world!   ",  11 },
    { 1, "[Hh]ello [Ww]orld\\s*[!]?", "Hello world  !",   13 },
    { 1, "[Hh]ello [Ww]orld\\s*[!]?", "hello World    !", 15 },
    { 0, "\\d\\d?:\\d\\d?:\\d\\d?",   "a:0",               0 },
    { 1, ".?bar",                     "real_bar",          4 },
    { 0, ".?bar",                     "real_foo",          0 },
    { 0, "X?Y",                       "Z",                 0 },
    { 1, "[a-z]+\nbreak",             "blahblah\nbreak",  14 },
    { 1, "[a-z\\s]+\nbreak",          "bla bla \nbreak",  14 },
    { 0, 0, 0, 0 }
};

int main(void) {
    int ntests;
    int nfailed;
    int i;
    int length;
    int m;
    const char *pattern;
    const char *text;

    ntests = 0;
    nfailed = 0;
    while (tests[ntests].pattern) {
        ntests += 1;
    }

    for (i = 0; i < ntests; ++i) {
        pattern = tests[i].pattern;
        text = tests[i].text;
        length = 0;
        m = tinyregex.re_match(pattern, text, &length);

        if (!tests[i].should_match) {
            if (m != -1) {
                runtime.printf("\n");
                tinyregex.re_print(tinyregex.re_compile(pattern));
                runtime.printf("[%d/%d]: pattern '%s' matched '%s' unexpectedly, matched %d chars.\n",
                               i + 1, ntests, pattern, text, length);
                nfailed += 1;
            }
        } else {
            if (m == -1) {
                runtime.printf("\n");
                tinyregex.re_print(tinyregex.re_compile(pattern));
                runtime.printf("[%d/%d]: pattern '%s' didn't match '%s' as expected.\n",
                               i + 1, ntests, pattern, text);
                nfailed += 1;
            } else if (length != tests[i].matchlen) {
                runtime.printf("[%d/%d]: pattern '%s' matched '%d' chars of '%s'; expected '%d'.\n",
                               i + 1, ntests, pattern, length, text, tests[i].matchlen);
                nfailed += 1;
            }
        }
    }

    runtime.printf("%d/%d tests succeeded.\n", ntests - nfailed, ntests);
    return nfailed;
}
