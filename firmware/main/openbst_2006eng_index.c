#include <string.h>

#include "bst_text.h"

#ifndef OPENBST_BUILD_NAME
#define OPENBST_BUILD_NAME "2006ENG"
#define OPENBST_DATA_SYMBOL BST_DATA_2006ENG
#endif

extern const bst_lifted OPENBST_DATA_SYMBOL;

const bst_lifted *bst_lifted_for(const char *build)
{
    return build && strcmp(build, OPENBST_BUILD_NAME) == 0
        ? &OPENBST_DATA_SYMBOL : NULL;
}
