#include<iostream>
#include<thread>
#include<vector>
#include<atomic>
#include<chrono>

struct MainNode;
struct KeyType;
struct ValueType;
struct INode;
struct SNode;
struct CNode;
struct AnyRef;

struct KeyType 
{
	// generics
};

struct ValueType 
{
	// generics
};

enum NodeType
{
	t_INode,
	t_CNode,
	t_SNode
};

class TNode
{
	 NodeType type;
	 
	 public:
		void setNodeType (NodeType type)
		{
			this->NodeType = type;
		}
		
		bool insert(KeyType key, ValueType value, int hashCode, int level, TNode parent)
		{
			if( this->NodeType == t_INode)	//is INode?
			{
				if (this.isNull())	//points to null?
				{
					//null INode
				}
				else
				{
					CNode next = this->main;
					// Calculate position in bitmap
					int index = (hashCode >> level) & 0x1f;
					int bitMap = next.bitMap;
					int flag = 1 << index;
					int mask = flag - 1;
					std::bitset<32> foo (bitMap & mask);
					int position = foo.count();

					if((bitMap & flag) != 0)
					{
						// There is a binding at the positition and it's not null - descend
						next.array[pos].insert(key, value, hashCode, level + 5, this);
					}
					else
					{
						// No binding at position - create a new node
						int length = next.array.length();
						INode *arr = ;
					}
				}
			}
		}
		
		TNode lookup(KeyType k, int hashCode, int level, TNode m, INode parent)
		{
			switch(m->NodeType)
			{
				case t_CNode:
				{
					int index = (hashCode >> level) & 0x1f);
					int bitMap = m.bitMap;
					int flag = 1 << index;
					if((bitMap & flag) == 0)	// Bitmap shoes no binding
						return NULL;
					else	// bitmap contains a value - descend
					{
						std::bitset<32> foo (bitMap & (flag-1));
						int position = foo.count();
						int subINode = m.array[position];
						subINode.lookup(key, hashCode, level + 5, subINode.main, this);
					}
					break;
				}
				case t_SNode:
				{
					if(!m.tomb)	// Singleton node
					{
						if(m.hashCode == hashCode && m->key == key)
							return (AnyRef) sn.v;
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

		ValueType remove(KeyType key, int hashCode, int level, INode parent)
		{
			INode m = parent.main;

			switch(m->NodeType)
			{
				case t_SNode:
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
				case t_CNode:
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
}

class INode: public TNode
{
    TNode* main;
	// can be an SNode or CNode
	
	public:
		INode()
		{
			setNodeType(t_INode);
		}
		
		bool isNull()
		{
			//implicit check against nullptr
			if(main)
				return true;
			else
				return false;
		}
};

class SNode: public TNode
{
	KeyType key;
	ValueType value;
    bool tomb;

	public:
		SNode(KeyType key, ValueType value, bool tomb)
		{
			setNodeType(t_SNode);
			this->key = key;
			this->value= value;
			this->tomb = tomb;
		}
};

class CNode: public TNode
{
    int bitMap;
    std::vector<INode*> array;  // branch factor = 2^W

	public:
		CNode(SNode n)
		{
			setNodeType(t_CNode);
		}
};


INode* root;

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
