#include "safeptr.h"


void	SafePtrList::ClearSafePtrs(void) {
	list < SafePtrHandle * >::iterator	iter = safePtrHandles.begin();
	list < SafePtrHandle * >::iterator	end = safePtrHandles.end();
	
	while (iter != safePtrHandles.end() && *iter) {
		(*iter)->spb_ptr = NULL;
		++iter;
	}
}


