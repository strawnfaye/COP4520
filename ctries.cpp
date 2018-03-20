#include<iostream>
#include<thread>
#include<vector>
#include<atomic>
#include<chrono>

struct INode 
{
    // main: MainNode
};

struct SNode 
{
    int keyType;
    int valueType;
    bool tomb;

    SNode(int keyType, int valueType, bool tomb)
    {
        this->keyType = keyType;
        this->valueType = valueType;
        this->tomb = tomb;
    }
};

struct CNode 
{
    int bitMap;
    std::vector<SNode*> array;  // branch factor = 2^W

    CNode()
    {
        
    }
};

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
