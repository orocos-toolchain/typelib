#ifndef TEST_HEADER_DATA_POSTERLIB_H
#define TEST_HEADER_DATA_POSTERLIB_H

#define STATUS int

typedef enum H2_ENDIANNESS {
  H2_BIG_ENDIAN,
  H2_LITTLE_ENDIAN
} H2_ENDIANNESS;

/* -- STRUCTURES ------------------------------------------ */
extern STATUS	logInit(int fd, int maxMsgs);
extern int logMsg ( const char *fmt, ... );

/* type of operation in posterTake() */
typedef enum {
	POSTER_READ,
	POSTER_WRITE,
	POSTER_IOCTL
} POSTER_OP;

#endif /*TEST_HEADER_DATA_POSTERLIB_H*/
