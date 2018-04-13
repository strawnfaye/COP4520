#include<iostream>
#include<thread>
#include<vector>
#include<atomic>
#include<chrono>

enum NodeType
{
	t_INode,	//indirection node
	t_CNode,	//array node with map and pointers
	t_SNode		//singleton node
};

template <class T>
class KeyType
{
	T value;
	int hashCode;

	KeyType(T value)
	{
		this->value = value;
		this->hashCode = computeHash(value);
	}

	T computeHash(T key)
	{
		// TODO: compute hashcode from key
	}
};


class Node
{
	 NodeType type;
	 
	 public:
		void setNodeType (NodeType type)
		{
			this->type = type;
		}
};

template <class T>
class INode: public Node
{
    Node* main;
	// can be an SNode or CNode
	
	public:
		INode(Node main)
		{
			setNodeType(t_INode);
			this->main = &main;
		}
		
		bool insertHelp(KeyType key, int level, Node parent)
		{
			Node focus = *main;
			
			if (focus == null)
			{
				clean(parent);
				return false;
			}
			else if (focus.type == t_CNode)
			{
				CNode cNext = focus;
				// Calculate position in bitmap
				int index = (hashCode >> level) & 0x1f;
				int bitMap = cNext.bmp;
				int flag = 1 << index;
				int mask = flag - 1;
				std::bitset<32> foo(bitMap & mask);
				int position = foo.count();

				if ((bitMap & flag) != 0)
				{
					// There is a binding at the positition and it's not null - descend
					CNode *cPoint = main;
					return cPoint.array[position].insert(key, level + 5, this);
				}
				else
				{
					// No binding at position - create a new node
					int length = arrayLength(cNext.array);
					INode *arr = new INode[length + 1];
					arrayCopy(cNext.array, arr);				//TODO: copy function to specifically skip [position]
					arr[position] = new SNode();				//TODO: SNode constructor (both SNode and CNode need to redirect to INode and bind main)
					CNode cNew = new CNode(bmp | flag, arr);
					*main = cNew;
					return true;
				}
			}
			else if (focus.type == t_SNode)
			{
				SNode sNext = main;
				if (!sNext.tomb)
				{
					if (sNext.key == key)
						return true;
					else
					{
						*main = new CNode(new SNode(key), sNext);	//TODO: CNode constructor to make CNode out of two SNodes
						return true;
					}
				}
				else
				{
					//tomb node-must be updated through parent and reattempted
					clean(parent);
					return false;
				}
			}
		}
};

template <typename T>
class SNode: public Node
{
	KeyType<T> key;
	NodeType type;
	bool tomb;

	SNode(T val, NodeType type, bool tomb)
	{
		this->type = type;
		this->key = new KeyType(val);
		this->tomb = tomb;
	}
};


class CNode: public Node
{
	int bmp;
	T array[];

	CNode(int bmp, T *array)
	{
		this->bmp = bmp;
		this->array = array;
	}
};

Node lookup(KeyType k, int hashCode, int level, Node m, INode parent)
{
	switch (m->NodeType)
	{
	case t_CNode:
	{
		int index = (hashCode >> level) & 0x1f);
		int bitMap = m.bitMap;
		int flag = 1 << index;
		if ((bitMap & flag) == 0)	// Bitmap shoes no binding
			return NULL;
		else	// bitmap contains a value - descend
		{
			std::bitset<32> foo(bitMap & (flag - 1));
			int position = foo.count();
			int subINode = m.array[position];
			subINode.lookup(key, hashCode, level + 5, subINode.main, this);
		}
		break;
	}
	case t_SNode:
	{
		if (!m.tomb)	// Singleton node
		{
			if (m.hashCode == hashCode && m->key == key)
				return (AnyRef)sn.v;
			else
				return NULL;
		}
		else	// Non-live node
		{
			clean(parent);
			// throw restartexception
		}
		break;
	}
	/*
	case NULL:
	{
	if(parent != NULL)
	{
	clean(parent);
	// throw restartexception
	}
	else
	return NULL;
	}
	*/
	//case null unreachable: calling method on null object results in runtime exception
	}
}

bool insert(KeyType key, ValueType value, int hashCode)
{
	if (root == null)
	{
		*root = new INode(new SNode(key, value, false));
	}
	else if (root->main == null)
	{
		//if root's INode points to null retry insert from null root
		*root = null;
		insert(key, value, hashCode);
	}
	else
	{
		//root and root reference are valid, insert as normal
		*root.insertHelp(key, value, hashCode, 0, null);
	}
}



T remove(KeyType key, int hashCode, int level, INode parent)
{
	INode m = parent.main;

	switch(m)
	{
		case SNode:
		{
			if(!m.tomb)	// Singleton node
			{
				if(m.hashCode == hashCode && m.key == key)
				{
					if(CAS(m, NULL))
						return m.value;
					else
						return null;
				}
				else
				{
					return null;
				}
			}
			break;
		}
		case CNode:
			int index = (hashCode >> level) & 0x1f);
			int bitMap = m.bitMap;
			int flag = 1 << index;
			if((bitMap & flag) == 0)	// Binding not found
				return NULL;
			else
			{
				std::bitset<32> foo (bitMap & (flag-1));
				int position = foo.count();
				int res = m.array[position].remove(key, hashCode, level + 5, this);

				// Start compression
				// Yikes, scala
			}
	}

}

template <typename T>
int arrayLength(T arr[])
{
	return (sizeof(arr))/(sizeof(arr[0]));
}

template <typename T>
0
void arrayCopy (T src[], T dest[])
{
	int srcLength = arrayLength(src);
	for (int i=0; i<srcLength; i++)
	{
		dest[i] = src[i];
	}
}

/*

Algorithm:

1. read root
	if root is null, tree is empty (key not found for removal/lookup)
	if root-> null INode, root set back to null before repeating
	(in both cases, insertion replaces root reference with a new CNode w appropriate key)


Insert
1. read root
	if root is null, tree is empty
		replace root reference with new CNode with appropriate key
	if root points to INode set to null, root set back to null before repeating
		replace root reference with new CNode with appropriate key
2. 	else	// read next node, below INode
		if CNode 
			// appropriate entry in its array must be found, flagpos(): computes flag and 				// position. 
			if(flag) // appropriate branch is in CNode
				pos is used as an index into array
			else // branch not in CNode
				creat updated copy of current CNode with new key
		if INode
			repeat operation recursively
		if SNode // key-value binding
			if keys are the same
				replace old binding
			else
				extend trie below CNode
	

Lookup
1. read root
	if root is null, tree is empty
	if root points to INode set to null, root set back to null before repeating
2. 	else	// read next node, below INode
		if CNode 
			// appropriate entry in its array must be found, flagpos(): computes flag and 				// position. 
			if(flag) // appropriate branch is in CNode
				pos is used as an index into array
			else // branch not in CNode
				terminate
		if INode
			repeat operation recursively
		if SNode // key-value binding
			if keys are the same
				return binding
			else
				terminate?

Removal
1. read root
	if root is null, tree is empty
	if root points to INode set to null, root set back to null before repeating
2. 	else	// read next node, below INode
		if CNode 
			// appropriate entry in its array must be found, flagpos(): computes flag and 				// position. 
			if(flag) // appropriate branch is in CNode
				pos is used as an index into array
			else // branch not in CNode
				terminate
		if INode
			repeat operation recursively
		if SNode // key-value binding
			if keys are the same
				replace CNode with its updated version without the key
				// Now trie has to be contracted
				read node below current INode
				if still a CNode
					toWeakTombed() // creates "weak tomb" from given CNode
					{
						if(numNodesBelowCNode(!nullINodes) > 1)
							this is the CNode itself, nothing to entomb
						if(above == 0)
							weak tomb = null
						else
							if(single branch below CNode is a key-value 											binding || is a tomb-I-Node (aka singleton))
								weak tomb is the tomb node with that binding
							if above is another CNode
								weak tomb is a copy of current CNode with null-INodes removed
					}
					loop tombCompress with CAS
					// continually tries to entomb current CNode until succeeds or finds there is nothing to 					//	entomb
					if CAS succeeds && weak tomb == null or == tomb node
						return true // parent node should be contracted
						contractParent() checks if INode is still reachable from parent
							contracts CNode below parent
							removes INode or resurrects a tomb-I-node into an SNode
					
				
			else
				terminate?


*/
