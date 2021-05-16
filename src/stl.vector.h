/***************************************************************************\
[*]      __     __  ___ __  _____  __     |        Part of LexiMUD        [*]
[*]     / /  ___\ \/ (_) _\/__   \/ /     |                               [*]
[*]    / /  / _ \\  /| \ \   / /\/ /      | Copyright(C) 1997-1999        [*]
[*]   / /__|  __//  \| |\ \ / / / /___    | All rights reserved           [*]
[*]   \____/\___/_/\_\_\__/ \/  \____/    | Chris Jacobson (FearItself)   [*]
[*]   LexiMUD Standard Template Library   | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : stl.vector.h                                                   [*]
[*] Usage: Vector template                                                [*]
\***************************************************************************/



#ifndef __STL_VECTOR_H__
#define __STL_VECTOR_H__



template <class T> class Vector;
template <class T> class VectorIter;


template <class T>
class Vector {
public:
						Vector(void);
						Vector(const Vector<T> &v);
						~Vector(void);
					
	void				Append(const T & item);
	void				Prepend(const T & item);
	void				Add(const T & item);
	void				Insert(const T & item, int nIndex);
//	void				InsertAfter(T item, T after);
//	void				InsertBefore(T item, T before);
	bool				Remove(int item);
	bool				Remove(const T &item);
	int					Find(const T &item) const;
//	void				UpdateIters(void);
	T &					Top(void) const;
	T &					Bottom(void) const;
	int					Count(void) const;
	bool				Contains(const T & item) const;
	void				Clear(void);
	void				Erase(void);
	
	void				SetSize(int newCount);
	
	T &					operator[](int nIndex) const;

	Vector<T> &			operator=(const Vector<T> &v);
protected:
	T *					table;
	int					count, size;
	
	void				Resize(int newSize);
};


template<class T>
Vector<T>::Vector(void) : table(new T[5]), count(0), size(5) {
}


template<class T>
Vector<T>::Vector(const Vector<T> &v) : table(new T[v.size]), count(v.count), size(v.size) {
//	if (!&v)	return;
	for (int element = 0; element < v.count; element++)
		table[element] = v.table[element];
}


template<class T>
Vector<T>::~Vector(void) {
	delete [] table;
}


template<class T>
void Vector<T>::Append(const T & item) {
	if (count >= size)	Resize(size + 5);
	table[count++] = item;
}


template<class T>
void Vector<T>::Prepend(const T & item) {
	if (count >= size)	Resize(size + 5);
	
	for (int x = count++; x > 0; x--)
		table[x] = table[x - 1];
	table[0] = item;
}


template<class T>
void Vector<T>::Add(const T & item) {
	this->Append(item);
}


template<class T>
void Vector<T>::Insert(const T & item, int nIndex) {
	if (count >= size)	Resize(size + 5);

	if (nIndex < 0)	nIndex = 0;
	if (nIndex > count) nIndex = count;

	for (int x = count++; x > nIndex; x--)
		table[x] = table[x - 1];
	table[nIndex] = item;
}


template<class T>
bool Vector<T>::Remove(int nIndex) {
	if (nIndex < 0 || nIndex > count)		return false;
	
	for (int i = nIndex; i < (count - 1); i++)
		table[i] = table[i + 1];
	
	count--;
	
	if (count < (size - 5))	Resize(size - 5);
	
	return true;
}


template<class T>
bool Vector<T>::Remove(const T &what) {
	int nIndex;
	
	for (nIndex = 0; nIndex < count; nIndex++) {
		if (table[nIndex] == what)
			break;
	}
	
	if (!count || nIndex >= count)		return false;
	
	for (; nIndex < (count - 1); nIndex++)
		table[nIndex] = table[nIndex + 1];
	
	count--;
	
	if (count < (size - 5))	Resize(size - 5);
	
	return true;
}


template<class T>
int Vector<T>::Find(const T &t) const {
	for (int i = 0; i < count; i++)
		if (table[i] == t)
			return i;
	
	return -1;
}


template<class T>
T &Vector<T>::Top(void) const {
	return table[count - 1];
}


template<class T>
T &Vector<T>::Bottom(void) const {
	return table[0];
}


template<class T>
int Vector<T>::Count(void) const {
	return count;
}


template<class T>
bool Vector<T>::Contains(const T & item) const {
	for (int i = 0; i < count; i++)
		if (table[i] == item)
			return true;
	return false;
}


template<class T>
void Vector<T>::Clear(void) {
	delete [] table;
	table = new T[5];
	size = 5;
	count = 0;
}


template<class T>
void Vector<T>::Erase(void) {
	table = new T[5];
	size = 5;
	count = 0;
}


template<class T>
T &Vector<T>::operator[](int nIndex) const {
	if (nIndex >= count)	nIndex = count - 1;
	if (nIndex < 0)			nIndex = 0;
	
	return table[nIndex];
}


template<class T>
void Vector<T>::SetSize(int newCount) {
	if (count == newCount || newCount == 0)	return;
	
	int	newSize = newCount;
	while (newSize % 5)	newSize++;
	Resize(newSize);

	count = newCount;
}


template<class T>
void Vector<T>::Resize(int newSize) {
	if (size == newSize || newSize == 0)	return;

	T *		newTable = new T[newSize];
	int		top = newSize > size ? size : newSize;
	
	for (int i = 0; i < top; i++)
		newTable[i] = table[i];
	
	delete [] table;
	table = newTable;
	size = newSize;
	
	if (count > size)	count = size;
}


template<class T>
Vector<T> &Vector<T>::operator=(const Vector<T> &v) {
	count = v.count;
	size = v.size;
	
	delete [] table;
	table = new T[v.size];

	for (int element = 0; element < count; element++)
		table[element] = v.table[element];
	
	return *this;
}


#endif

