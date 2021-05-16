#ifndef __MEMORY_H__
#define __MEMORY_H__

void Free_Error(char * file, int line);
#ifndef macintosh
inline void Free_Error(const char * file, int line) {	Free_Error(const_cast<const char *>(file), line);	}
#endif

#define ALLOC(a, b)		calloc(a, b)
						
#define FREE(ptr)		if (ptr == NULL) {						\
							Free_Error(__FUNCTION__, __LINE__);	\
						} else {								\
							free(ptr);							\
							ptr = NULL;							\
						}

#endif

