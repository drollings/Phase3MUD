/***************************************************************************\
[*]      __     __  ___ __  _____  __     |        Part of LexiMUD        [*]
[*]     / /  ___\ \/ (_) _\/__   \/ /     |                               [*]
[*]    / /  / _ \\  /| \ \   / /\/ /      | Copyright(C) 1997-1999        [*]
[*]   / /__|  __//  \| |\ \ / / / /___    | All rights reserved           [*]
[*]   \____/\___/_/\_\_\__/ \/  \____/    | Chris Jacobson (FearItself)   [*]
[*]   LexiMUD Standard Template Library   | fear@technologist.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] File : stl.map.h                                                      [*]
[*] Usage: Map template                                                   [*]
\***************************************************************************/


#ifndef __STL_MAP_H__
#define __STL_MAP_H__


//#include "stl.vector.h"


template<class K, class V>	class Map;
//template<class K, class V>	class MapIterator;



template<class KEY, class VALUE>
class Pair {
public:
						Pair(void) {}
						Pair(const Pair<KEY, VALUE> &p);
						Pair(const KEY& k, const VALUE& d);
						~Pair(void) {}
						
//	KEY &				Key(void)								{	return key;		}
//	VALUE &				Data(void)								{	return data;	}
//private:
				
	KEY					key;
	VALUE				data;
	
	Pair<KEY, VALUE> &	operator=(const Pair<KEY, VALUE> &p);
/*	bool				operator==(const Pair<KEY, VALUE> &p); */
	bool				operator<(const Pair<KEY, VALUE> &p) { return (key < p.key); }
};


template<class KEY, class VALUE>
Pair<KEY, VALUE>::Pair(const KEY& k, const VALUE& v) : key(k), data(v) { }


template<class KEY, class VALUE>
Pair<KEY, VALUE>::Pair(const Pair<KEY, VALUE> &p) : key(p.key), data(p.data) { }
				

template<class K, class V>
Pair<K, V> &Pair<K, V>::operator=(const Pair &p) {
	key = p.key;
	data = p.data;
	
	return *this;
}


template<class K, class V>
class Node {
public:
					Node(void) : left(NULL), right(NULL), parent(NULL), bal(0) { }
	Node *left, *right, *parent;
	
	Pair<K, V>		pair;
	int				bal;
	
	Node *			add(Node *node);
	Node *			remove(Node *node);
	
private:
	Node *			singleRotateLeft(void);
	Node *			singleRotateRight(void);
	Node *			restoreLeftBalance(int bal);
	Node *			restoreRightBalance(int bal);
	Node *			balance(void);
	Node *			removeLeftChild(Node *&ref);
};


template <class K, class V>
class MapIterator {
public:
	friend class Map<K, V>;
	
	typedef typename Map<K, V>::MapPair	MapPair;
	typedef typename Map<K, V>::Node		typeNode;
						MapIterator(void);
						MapIterator(Map<K, V> &theMap);
						~MapIterator(void);
	void				Start(Map<K, V> &theMap);
	void				Finish(void);
	void				Restart(Map<K, V> &theMap);
	void				Update(typeNode *node);
	
	V *					Peek(void) const;
	V *					Prev(void);
	V *					Next(void);
	void				Reset(void);

protected:
	Map<K, V> *			map;
	typeNode *				current;
//	MapIterator<K, V> *	next;
};


template<class K, class V>
class Map {
//	friend class MapIterator<K, V>;
public:
	typedef Pair<K, V>	MapPair;
	typedef Node<K, V>	typeNode;
	typedef MapIterator<K, V>	Iterator;
	
						Map(void);
//						Map(const Map &m);
						~Map(void);
	
	Map<K, V> &			Insert(typeNode *node);
	Map<K, V> &			Remove(const K &key);
//	const T &			operator[](const I &key) const;
	V &					operator[](const K &key);
	Map<K, V> &			operator=(const Map<K, V> &m);
	Map<K, V> &			Move(const K &from, const K &to);
	Map<K, V> &			Swap(const K &a, const K &b);
	
	const K				Top(void);
	const K				Bottom(void);
	int					Count(void);
	
	bool				Find(const K &key) const;
	typeNode *				GetElement(const K &key) const;
	
public:
	typeNode *				root;
	int					count;
	mutable typeNode *		cache;
};


template<class K, class V>
Map<K, V>::Map(void) : root(NULL), count(0), cache(0) /* : iters(NULL) */ {
}

/*
template<class K, class V>
Map<K, V>::Map(const Map &m) : map(m.vector)  {
}
*/

template<class K, class V>
Map<K, V>::~Map(void) {
}


//	Perform a single left rotation of current node
template<class K, class V>
Node<K, V> *Node<K, V>::singleRotateLeft(void) {
	Node *		node = right;
	
	//	Make reconnections
	right = node->left;
	node->left = this;
	
	if (right)	right->parent = this;
	parent = node;
	
	int Abf = bal;
	int Bbf = node->bal;
	
	if (Bbf <= 0) {
		node->bal = (Abf >= 1) ? (Bbf - 1) : (Abf + Bbf - 2);
		bal = Abf - 1;
	} else {
		node->bal = (Abf <= Bbf) ? (Abf - 2) : (Bbf - 1);
		bal = (Abf - Bbf) - 1;
	}
	return node;
}


//	Perform a single right rotation of current node
template<class K, class V>
Node<K, V> *Node<K, V>::singleRotateRight(void) {
	Node *		node = left;
	
	//	Make reconnections
	left = node->right;
	node->right = this;
	
	if (left)	left->parent = this;
	parent = node;
	
	int Abf = bal;
	int Bbf = node->bal;
	
	if (Bbf <= 0) {
		node->bal = (Bbf > Abf) ? (Bbf + 1) : (Abf + 2);
		bal = (Abf - Bbf) + 1;
	} else {
		node->bal = (Abf <= -1) ? (Bbf + 1) : (Abf + Bbf + 2);
		bal = Abf + 1;
	}
	return node;
}


//	Balance tree rooted at node
//	Use single or double rotations as appropriate
template<class K, class V>
Node<K, V> *Node<K, V>::balance(void) {
	if (bal < 0) {
		if (left->bal <= 0)		return singleRotateRight();
		else {
			left = left->singleRotateLeft();
			left->parent = this;
			return singleRotateRight();
		}
	} else {
		if (right->bal >= 0)	return singleRotateLeft();
		else {
			right = right->singleRotateRight();
			right->parent = this;
			return singleRotateLeft();
		}
	}
}


//	Perform a single left rotation of current node
template<class K, class V>
Node<K, V> *Node<K, V>::add(Node *node) {
	if (node->pair.key < pair.key) {
		if (left) {
			int oldBalance = left->bal;
			left = left->add(node);
			//	Check to see if tree grew
			if ((left->bal != oldBalance) && left->bal)
				bal--;
		} else {
			left = node;
			bal--;
		}
		left->parent = this;
	} else {
		if (right) {
			int oldBalance = right->bal;
			right = right->add(node);
			//	Check to see if tree grew
			if ((right->bal != oldBalance) && right->bal)
				bal++;
		} else {
			right = node;
			bal++;
		}
		right->parent = this;
	}
	
	if ((bal < -1) || (bal > 1))
		return balance();
	
	return this;
}


//	Remove leftmost child
template<class K, class V>
Node<K, V> *Node<K, V>::removeLeftChild(Node *&node) {
	if (!left) {
		node = this;
		return right;
	}
	
	int oldBalance = left->bal;
	
	left = left->removeLeftChild(node);
	left->parent = this;
	
	return restoreLeftBalance(oldBalance);
}


//	Restore left balance
template<class K, class V>
Node<K, V> *Node<K, V>::restoreLeftBalance(int oldBalance) {
	if (!left)	bal++;
	else if ((left->bal == oldBalance) && left->bal == 0)
		bal++;
	
	if (bal > 1)	return balance();
	
	return this;
}


//	Restore right balance
template<class K, class V>
Node<K, V> *Node<K, V>::restoreRightBalance(int oldBalance) {
	if (!right)	bal--;
	else if ((right->bal == oldBalance) && right->bal == 0)
		bal--;
	
	if (bal < -1)	return balance();
	
	return this;
}


//	Remove selected node
template<class K, class V>
Node<K, V> *Node<K, V>::remove(Node *node) {
	if (node == this) {
		//	If no right child, return left
		if (!right)		return left;
		//	else find and remove smallest left child of the right child
		int oldBalance = right->bal;
		Node *	root;
		right = right->removeLeftChild(root);
		//	Connect new root
		root->left = left;		if (left)	left->parent = root;
		root->right = right;	if (right)	right->parent = root;
		root->bal = bal;
		return root->restoreRightBalance(oldBalance);
	} else if (node->pair.key < pair.key) {
		if (!left)		return this;
		//	do the deletion
		int oldBalance = left->bal;
		left = left->remove(node);
		if (left)	left->parent = this;
		return restoreLeftBalance(oldBalance);
	} else {
		if (!right)		return this;
		int oldBalance = right->bal;
		
		right = right->remove(node);
		if (right)	right->parent = this;
		return restoreRightBalance(oldBalance);
	}
}


template<class K, class V>
Map<K, V> &Map<K,V>::Insert(Node *node) {
	root = root ? root->add(node) : node;
	if (root)	root->parent = NULL;
	count++;
	return *this;
}


template<class K, class V>
Map<K, V> &Map<K,V>::Remove(const K &key) {
	Node *	node = GetElement(key);
	if (!node)	return *this;
	root = root->remove(node);
	if (root)	root->parent = NULL;
	count--;
	if (cache == node)	cache = NULL;
	delete node;
	return *this;
}


template<class K, class V>
V &Map<K, V>::operator[](const K &key) {
	Node *	node = GetElement(key);
	if (node)	return node->pair.data;
	
	node = new Node;
	node->pair.key = key;
	
	Insert(node);
	return node->pair.data;
}


template<class K, class V>
Map<K, V> &Map<K, V>::operator=(const Map<K, V> &m) {
//	vector = m.vector;
}


template<class K, class V>
Map<K, V> &Map<K, V>::Move(const K &from, const K &to) {
	Node *node = GetElement(from);
	if (!node)		return *this;
	root = root->remove(node);
	if (root)	root->parent = NULL;
	node->pair.key = to;
	root = root->add(node);
	root->parent = NULL;
	
	return *this;
}


template<class K, class V>
Map<K, V> &Map<K, V>::Swap(const K &a, const K &b) {
	Node *nodeA = GetElement(a);
	Node *nodeB = GetElement(b);
	
	if (!nodeA && !nodeB)	return *this;

	if (nodeA) {
		root = root->remove(nodeA);
		if (root)	root->parent = NULL;
	}
	if (nodeB) {
		root = root->remove(nodeB);
		if (root)	root->parent = NULL;
	}
	
	if (nodeA) {
		nodeA->pair.key = b;
		root = root.add(nodeA);
		root->parent = NULL;
	}
	if (nodeB) {
		nodeB->pair.key = a;
		root = root.add(nodeB);
		root->parent = NULL;
	}
	return *this;
}


template<class K, class V>
const K Map<K, V>::Bottom(void) {
	Node *node = root;
	if (!node)	return K();
	while (node->left)	node = node->left;
	return node->pair.key;
}


template<class K, class V>
const K Map<K, V>::Top(void) {
	Node *node = root;
	if (!node)	return K();
	while (node->right)	node = node->right;
	return node->pair.key;
}


template<class K, class V>
int Map<K, V>::Count(void) {
	return count;
}



//	Retrieve vector index of element by key
template<class K, class V>
Node<K, V> *Map<K, V>::GetElement(const K &key) const {
	if (cache && cache->pair.key == key)	return cache;
		
	Node *	node = root;
	while (node) {
		if (node->pair.key == key) {
			cache = node;
			return node;
		}
		if (key < node->pair.key)	node = node->left;
		else						node = node->right;
	}
	
	return NULL;
}


template<class K, class V>
bool Map<K, V>::Find(const K &key) const {
	return (GetElement(key) != NULL);
}


template<class K, class V>
MapIterator<K, V>::MapIterator(void) : map(NULL), current(NULL) /*, next(NULL) */ {
}


template<class K, class V>
MapIterator<K, V>::MapIterator(Map<K, V> &theMap) : map(NULL), current(NULL) /*, next(NULL) */ {
	this->Start(theMap);
}


template<class K, class V>
MapIterator<K, V>::~MapIterator(void) {
	this->Finish();
}


template<class K, class V>
void MapIterator<K, V>::Start(Map<K, V> &theMap) {
	if (map)		Finish();
	if (!(map = &theMap))	return;
//	next = theMap.iters;
//	theMap.iters = this;
	Reset();
}


template<class K, class V>
void MapIterator<K, V>::Finish(void) {
	if (!map)	return;
//	map->iters = next;
//	next = NULL;
	current = NULL;
	map = NULL;
}


template<class K, class V>
void MapIterator<K, V>::Restart(Map<K, V> &theMap) {
	Finish();
	Start(map);
}


template<class K, class V>
void MapIterator<K, V>::Update(Node *node) {
//	if (current == node)	node = NULL;
}


template<class K, class V>
V *MapIterator<K, V>::Peek(void) const {
	if (!map)	return NULL;
	return (current ? &current->pair.data : NULL);
}


template<class K, class V>
V *MapIterator<K, V>::Next(void) {
	Node *node = current;
	
	if (!node)	return NULL;
	
	if (current->right) {
		current = current->right;
		while (current->left)	current = current->left;
	} else {
		Node *link = current->parent;
		while (link && current == link->right) {
			current = link;
			link = link->parent;
		}
//		if (link && link)
		current = link;
	}
	
	return &node->pair.data;
}


template<class K, class V>
V *MapIterator<K, V>::Prev(void) {
//	if (!map)	return NULL;
//	return (--current > -1) ? &map->vector[current] : NULL;
	return NULL;
}


template<class K, class V>
void MapIterator<K, V>::Reset(void) {
	Node *node = map->root;
	if (node)	while (node->left)	node = node->left;
	current = node;
}


#endif

