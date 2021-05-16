/*************************************************************************
*   File: hash.h                     Part of Aliens vs Predator: The MUD *
*  Usage: Header and inlined code for hash tables                        *
*************************************************************************/


#ifndef __HASH_H__
#define __HASH_H__


#include "types.h"

#include "stl.vector.h"

template<class T> struct HashLink;
template<class T> class HashHeader;

/*
typedef void (*DestroyHashFunc)(Ptr data);
void DestroyHashHeader(HashHeader *ht, DestroyHashFunc gman);


typedef void (*HashIterateFunc)(SInt32 key, Ptr data, Ptr cdata);
void HashIterate(HashHeader* ht, HashIterateFunc func, Ptr cdata);
*/
template<class T>
class Hash {
protected:
	struct Link {
		SInt32			key;
		T				data;
	};

public:
					Hash(SInt32 table_size);
					Hash(SInt32 table_size, T &bad);
					
	typedef Vector<Link>	Bucket;
	
	SInt32			DoHash(SInt32 key);
	bool			Enter(SInt32 key, T &data);
	T				Remove(SInt32 key);
	T &				Find(SInt32 key);
	
	Vector<Bucket>	mBuckets;
	T				mBad;
protected:
	
	Link *			_find(SInt32 key);
	void			_enter(SInt32 key, T &data);
	void			_remove(SInt32 key);
};


template<class T>
Hash<T>::Hash(SInt32 tableSize) : mBad(T()) {
	mBuckets.SetSize(tableSize);
}


template<class T>
Hash<T>::Hash(SInt32 tableSize, T &bad) : mBad(bad) {
	mBuckets.SetSize(tableSize);
}


template<class T>
SInt32 Hash<T>::DoHash(SInt32 key) {
	return ((SInt32) key * 17) % mBuckets.Count();
}


template<class T>
T &Hash<T>::Find(SInt32 key) {
	Link *	item;
	
	item = _find(key);
	return item ? item->data : mBad;
}


template<class T>
bool Hash<T>::Enter(SInt32 key, T &data) {
	Link *item;
	
	if ((item = _find(key)))
		return false;
	
	_enter(key, data);
	
	return true;
}


template<class T>
Hash<T>::Link *Hash<T>::_find(SInt32 key) {
	Bucket &	bucket = mBuckets[DoHash(key)];
	SInt32		compare;

	for (int i = 0; i < bucket.Count(); i++) {
		compare = key - bucket[i].key;
		if (compare == 0)		return &bucket[i];
		else if (compare < 0)	break;
	}
	return NULL;
}


template <class T>
void Hash<T>::_enter(SInt32 key, T &data) {
	Bucket &	bucket = mBuckets[DoHash(key)];
	int			i;
	Link		link;
	
	link.key = key;
	link.data = data;
	
	for (i = 0; i < bucket.Count(); i++)
		if ((bucket[i].key - key) <= 0)
			break;

	bucket.Insert(link, i);
}


template<class T>
void Hash<T>::_remove(SInt32 key) {
	Bucket &	bucket = buckets[Hash(key)];
	
	for (int i = 0; i < bucket.Count(); i++) {
		if (bucket[i].key == key) {
			bucket.Remove(i);
			break;
		}
	}
}


#endif
