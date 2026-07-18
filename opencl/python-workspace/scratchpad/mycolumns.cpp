// this file is meant to be used by the parselog script
// it is a shitty reimplementation o' mine of a subset of gnu column
// born out of my frustration with trying to understand how to use gnu column

#include<string>
#include<vector>

#include<cassert>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<cctype>

#define SEPARATOR (',')
#define EXTRA_PADDING (3)
#define MAX(a,b)(a>b?a:b)

ssize_t index_of(const char c, const char* s) {
    const char* anchor = s;
    while(*s && *s != c) s++;
    if(*s)
        return s-anchor;
    return -1;
}

const char* cpy(const char* src, size_t len) {
    char* dst = (char*)calloc(len + 1, sizeof(char));
    memcpy(dst, src, len);
    dst[len]='\0';
    return dst;
}

int main() {
    std::vector<std::pair<const char*, size_t>> lines;

    char* line = NULL;
    size_t len = 0;
    while (getline(&line, &len, stdin) != -1) {
        ssize_t sep = index_of(SEPARATOR, line);
        if(sep < 0) sep = len;

        lines.push_back({cpy(line, len), (size_t)sep});
        free(line), line = NULL, len = 0 ;
    }

    size_t max_c1_w = 0;
    for(const auto&[s, i] : lines)
        max_c1_w = MAX(max_c1_w, i);

    for(const auto&[s, i] : lines) {
        for(const char* c = s; *c != SEPARATOR; c++) fputc(*c, stdout);
        for(size_t j = i; j<max_c1_w + EXTRA_PADDING; ++j) fputc(' ', stdout);
        for(const char* c = s+i+1; *c && !isspace(*c); c++) fputc(*c, stdout);
        puts("");
    }

    return 0;
}
