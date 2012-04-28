#define SYMBOL_NAME_STR(X) #X
#define SYMBOL_NAME(X) X

#ifdef __STDC__
#define SYMBOL_NAME_LABEL(X) SYMBOL_NAME(X)##:
#else
#define SYMBOL_NAME_LABEL(X) SYMBOL_NAME(X)/**/:
#endif


#ifdef __ASSEMBLY__
#define ENTRY(name) \
  .globl SYMBOL_NAME(name); \
  SYMBOL_NAME_LABEL(name)

#endif
