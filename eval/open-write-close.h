

#define NSAMPLES	(10)
#define WITERS		(100)
#define NITERS		(10*1000)

#ifdef AH
#define NSAMPLES	(10)
#define WITERS		(10)
#define NITERS		(10)
#endif

#define IOSZ0		(4096)
#define BUFSZ		(128*1024)
//#define BUFSZ		(4096*2)


#define min(X,Y)	( ((X)<(Y))?(X):(Y) )
#define max(X,Y)	( ((X)>(Y))?(X):(Y) )

extern void IMB_cpu_exploit( float, int );
