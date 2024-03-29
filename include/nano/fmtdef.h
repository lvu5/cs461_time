/* Copyright (c) 2002-2006 Lucent Technologies; see LICENSE */

/*
 * dofmt -- format to a buffer
 * the number of characters formatted is returned,
 * or -1 if there was an error.
 * if the buffer is ever filled, flush is called.
 * it should reset the buffer and return whether formatting should continue.
 */

typedef int (*Fmts)(Fmt*);

typedef struct Quoteinfo Quoteinfo;
struct Quoteinfo
{
	int	quoted;		/* if set, string must be quoted */
	int	nrunesin;	/* number of input runes that can be accepted */
	int	nbytesin;	/* number of input bytes that can be accepted */
	int	nrunesout;	/* number of runes that will be generated */
	int	nbytesout;	/* number of bytes that will be generated */
};

/* Edit .+1,/^$/ |cfn |grep -v static | grep __ */
double       __Inf(int sign);
double       __NaN(void);
int          __badfmt(Fmt *f);
int          __charfmt(Fmt *f);
int          __countfmt(Fmt *f);
int          __efgfmt(Fmt *fmt);
int          __errfmt(Fmt *f);
int          __flagfmt(Fmt *f);
int          __fmtFdFlush(Fmt *f);
int          __fmtcpy(Fmt *f, const void *vm, int n, int sz);
void*        __fmtdispatch(Fmt *f, void *fmt, int isrunes);
void *       __fmtflush(Fmt *f, void *t, int len);
void         __fmtlock(void);
int          __fmtpad(Fmt *f, int n);
double       __fmtpow10(int n);
int          __fmtrcpy(Fmt *f, const void *vm, int n);
void         __fmtunlock(void);
int          __ifmt(Fmt *f);
int          __isInf(double d, int sign);
int          __isNaN(double d);
int          __needsep(int*, char**);
int          __needsquotes(char *s, int *quotelenp);
int          __percentfmt(Fmt *f);
int          __quotestrfmt(int runesin, Fmt *f);
int          __rfmtpad(Fmt *f, int n);
int          __runefmt(Fmt *f);
int          __runesfmt(Fmt *f);
int          __strfmt(Fmt *f);

#define FMTCHAR(f, t, s, c)\
	do{\
	if(t + 1 > (char*)s){\
		t = (char*)__fmtflush(f, t, 1);\
		if(t != nil)\
			s = (char*)f->stop;\
		else\
			return -1;\
	}\
	*t++ = c;\
	}while(0)

#ifdef va_copy
#	define VA_COPY(a,b) va_copy(a,b)
#	define VA_END(a) va_end(a)
#else
#	define VA_COPY(a,b) (a) = (b)
#	define VA_END(a)
#endif

