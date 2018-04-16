#include "stdafx.h"
#include<iostream>
#include<thread>
#include<vector>
#include<atomic>
#include<chrono>
#include<functional>
#include "seq.h"

// go through and change operations from sequential to atomic
// why are bitset and foo not defined?

// Uses bitwise operations and popcount emulation to find appropriate index in 
// a CNode's array through its bitmap.
int CTrie::calculateIndex(KeyType key, int level, CNode *cn)
{
	int index = (key.hashCode >> level & 0x1f);
	int bitMap = cn->bmp;
	int flag = 1 << index;
	int mask = flag - 1;
	std::bitset<LENGTH> foo(bitMap & mask);
	return foo.count();
}

bool CTrie::insert(int val)
{
	KeyType key = KeyType(val);
	NodePtr currRoot;
	currRoot.in = root;
	currRoot.type = t_INode;
	std::cout << "currRoot.in = " << currRoot.in << ".\n";
	// If root is null or its next pointer is null, then tree is empty.
	if (currRoot.in == NULL || (currRoot.in->main.isNull))
	{
		std::cout << "Tree is empty.\n";
		// Create new CNode that contains the new SNode with the key, and a new INode to point to it.
		NodePtr cnPtr;
		cnPtr.type = t_CNode;
		cnPtr.cn = new CNode;
		cnPtr.isNull = false;
		NodePtr snPtr;
		snPtr.type = t_SNode;
		snPtr.sn = (new SNode(key, t_SNode, false));
		snPtr.isNull = false;
		NodePtr inPtr;
		inPtr.type = t_INode;
		inPtr.isNull = false;

		// change root pointer to concurrent
		do
		{
			//INode oldRoot = *root;
			int i = calculateIndex(key, 0, cnPtr.cn);
			std::cout << "Inserting " << key.value << " into array at root level and index " << i << ".\n";
			cnPtr.cn->addToArray(i, snPtr);
			// Initialize new INode's main to the new CNode.
			inPtr.in = (new INode(t_CNode, cnPtr));
			// TODO: Compare and swap INode at root.
			// code below is obsolete
			root = inPtr.in;
			// while(!root.compare_exchange_weak(oldRoot, inPtr.in)) vs strong?
		} while (root != inPtr.in);
		std::cout << "Successful insertion at root. Root is pointing to a " << root->type << "\n";
		return true;
	}
	// Root points to valid INode.
	else
	{
		while (!iinsert(currRoot, key, 0, currRoot))
		{
			std::cout << "recursing again.\n";
			return insert(val);
		}
		std::cout << "Successful insertion.\n";
	}
	std::cout << "bottom of insert, whoops.\n";
	return true;
}

bool CTrie::iinsert(NodePtr curr, KeyType key, int level, NodePtr parent)
{
	std::cout << "in iinsert(), curr.type is " << curr.type << "\n";
	switch (curr.type)
	{
	case t_CNode:
	{
		// Appropriate entry in array must be found by calculating position in bitmap.
		int index = (key.hashCode >> level & 0x1f);
		unsigned int bitMap = curr.cn->bmp;
		int flag = 1 << index;
		int mask = flag - 1;
		std::bitset<LENGTH> foo(bitMap & mask);
		int position = foo.count();

		// If there is a binding at the position in CNode and it's not null, repeat operation recursively.
		if ((bitMap & flag) != 0)
		{
			std::cout << "Collision at " << position << " and level " << level << "; recursing down.\n";
			return iinsert(curr.cn->array[position], key, level + W, curr);
		}
		// No binding at position
		else
		{
			std::cout << "No binding at position, adding to CNode.\n";
			// Create an updated version of CNode containing new key.
			NodePtr temp[32] = {};
			int newBitMap;

			NodePtr cnPtr;
			cnPtr.cn = new CNode;
			cnPtr.type = t_CNode;
			cnPtr.isNull = false;

			NodePtr snPtr;
			snPtr.sn = new SNode(key, t_SNode, false);
			snPtr.type = t_SNode;
			snPtr.isNull = false;

			// Update array and bitmap.
			curr.cn->copyArray(temp);
			newBitMap = curr.cn->bmp;
			newBitMap |= flag;
			cnPtr.cn->bmp = newBitMap;
			cnPtr.cn->addToArray(position, snPtr);
			// TODO: CAS on curr
			// change code below
			// curr = CAS(cnPtr);
			curr = cnPtr;
			return(true);
		}
		break;
	}
	case t_INode:
	{
		std::cout << "Recursing bc INode.\n";
		// Repeat operation recursively.
		return iinsert(curr.in->main, key, level + W, curr);
		break;
	}
	case t_SNode:
	{
		// If this is a tomb node, perform a clean() operation and return.
		if (curr.sn->tomb)
		{
			if (!parent.isNull)
			{
				//clean(parent);
				std::cout << "Would be cleaning here.\n";
			}
			return true;
		}
		else
		{
			int currHash = curr.sn->key.hashCode;
			// If this is the same value, replace it with a new binding.
			if (currHash == key.hashCode && curr.sn->key.value == key.value)
			{
				std::cout << "Collision but values are the same.\n";
				NodePtr snPtr;
				snPtr.type = t_SNode;
				snPtr.sn = new SNode(key, t_SNode, false);
				snPtr.isNull = false;
				// TODO: CAS on curr
				// curr = CAS(snPtr);
				curr = snPtr;
				return(true);
			}
			// Different value but same hash prefix, so extend tree one level down with both keys.
			else
			{
				std::cout << "Same hash prefix, extending tree.\n";
				// Create new CNode containing current and new SNodes
				NodePtr cn2Ptr;
				cn2Ptr.type = t_CNode;
				cn2Ptr.cn = new CNode;
				cn2Ptr.isNull = false;

				NodePtr snPtr;
				snPtr.type = t_SNode;
				snPtr.isNull = false;
				cn2Ptr.sn = new SNode(key, t_SNode, false);

				int i = calculateIndex(key, level, cn2Ptr.cn);
				int j = calculateIndex(curr.sn->key, level, cn2Ptr.cn);
				cn2Ptr.cn->addToArray(i, snPtr);
				cn2Ptr.cn->addToArray(j, curr);

				// Create new INode to point to new CNode
				NodePtr inPtr;
				inPtr.type = t_INode;
				inPtr.in = new INode(t_CNode, cn2Ptr);
				inPtr.isNull = false;

				// Create updated version of parent CNode so that it points to new INode at position SNode is moving from.
				NodePtr cn1Ptr;
				cn1Ptr.type = t_CNode;
				cn1Ptr.cn = new CNode;
				cn1Ptr.isNull = false;

				NodePtr tempArr[32] = {};
				parent.cn->copyArray(tempArr);
				int bitMap = parent.cn->bmp;
				i = calculateIndex(curr.sn->key, level, cn1Ptr.cn);
				cn1Ptr.cn->addToArray(j, inPtr);
				// TODO: CAS on parent CNode
				// parent = CAS(cn1Ptr);
				parent = cn1Ptr;
				std::cout << "Successful extension.\n";
				return(true);
			}
		}
		break;
	}
	default:
		return true;
	}
}

int main(void)
{
	std::cout << "ayyy in program.\n";
	CTrie::CTrie myTrie;
	std::cout << "myTrie is supposedly created.\n";
	myTrie.insert(1);
	std::cout << "Returned from insert(1).\n";
	myTrie.insert(2);
	std::cout << "Returned from insert(2).\n";

	return 0;
}