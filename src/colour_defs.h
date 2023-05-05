#ifndef COLOUR_DEFS_H_INCLUDED
#define COLOUR_DEFS_H_INCLUDED


#define __DEF_COL(i) "\x1B[38;5;"#i"m"

#define COLOUR_ERROR __DEF_COL(1)
#define COLOUR_INFO __DEF_COL(6)

#define COLOUR_THEME1 __DEF_COL(5)
#define COLOUR_THEME2 __DEF_COL(3)

#define COLOUR_HIGHLIGHT1 __DEF_COL(15)

#define COLOUR_RESET "\x1B[0m"

#endif

