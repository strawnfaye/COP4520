#include<iostream>
#include<thread>
#include<vector>
#include<atomic>
#include<chrono>
#include<functional>
#include "seq.h"

// Uses bitwise operations and popcount emulation to find appropriate index in 
// a CNode's array through its bitmap.
int CTrie::calculateIndex(KeyType key, int level, CNode *cn)
{
    int index = (key.hashCode >> level & 0x1f);
    //int bitMap = cn->bmp;
    //int flag = 1 << index;
    //int mask = flag - 1;
    //std::bitset<LENGTH> foo(bitMap & mask);
    //return foo.count();
    return index;
}

CNode *initCNode()
{
    CNode *temp = new CNode;
    temp->initArray();
    temp->bmp = 0;
    return temp;
}

SNode *initSNode(KeyType key)
{
    SNode *temp = new SNode(key, t_SNode, false);
    return temp;
}

NodePtr initNodePtr(NodeType type, KeyType key)
{
    NodePtr temp;
    temp.isNull = false;
    temp.type = type;
    if(type == t_CNode)
        temp.cn = initCNode();
    else if(type == t_SNode)
        temp.sn = initSNode(key);
    return temp;
}

bool CTrie::insert(int val)
{
    KeyType key = KeyType(val);
    NodePtr currRoot;
    currRoot.in = root;
    currRoot.type = t_INode;
    std::cout << "currRoot.in = " << currRoot.in << ".\n";
    // If root is null or its next pointer is null, then tree is empty.
    if(currRoot.in == NULL || (currRoot.in->main.isNull))
    {
        std::cout << "Tree is empty.\n";
        // Create new CNode that contains the new SNode with the key, and a new INode to point to it.
        NodePtr cnPtr = initNodePtr(t_CNode, key);
        NodePtr snPtr = initNodePtr(t_SNode, key);
        NodePtr inPtr = initNodePtr(t_INode, key);

        do
        {
            int i = calculateIndex(key, 0, cnPtr.cn);
            std::cout << "Inserting " << key.value << " into array at root level and index " << i << ".\n";
            cnPtr.cn->addToArray(i, snPtr);
            snPtr.sn->parent = cnPtr;
            // Initialize new INode's main to the new CNode.
            inPtr.in = (new INode(t_CNode, cnPtr));
            cnPtr.cn->parentINode = inPtr.in;
            
            // TODO: Compare and swap INode at root.
            root = inPtr.in;
        } while(root != inPtr.in);
        std::cout << "Successful insertion at root. Root is pointing to a " << root->type << "\n";
        return true;    
    }
    // Root points to valid INode.
    else 
    {
        while(!iinsert(currRoot, key, -1, &root))
        {
            std::cout << "recursing again.\n";
            return insert(val);
        }
        std::cout << "Successful insertion.\n";
    }
    std::cout << "bottom of insert, whoops.\n";
    return true;
}

bool CTrie::iinsert(NodePtr curr, KeyType key, int level, INode **parent)
{
    std::cout << "in iinsert(), curr.type is " << curr.type <<"\n";
    switch(curr.type)
    {
        case t_CNode:
        {
            // Appropriate entry in array must be found by calculating position in bitmap.
            // int index = (key.hashCode >> level & 0x1f);
            // unsigned int bitMap = curr.cn->bmp;
            // int flag = 1 << index;
            // int mask = flag - 1;
            // std::bitset<LENGTH> foo(bitMap & mask);
            // int position = foo.count();
            int position = calculateIndex(key, level, curr.cn);

            // If there is a binding at the position in CNode and it's not null, repeat operation recursively.
            // TODO: fix logic on (bitMap & flag) != 0 
            if(!curr.cn->array[position].isNull)
            {
                std::cout << "Collision at " << position << " and level " << level << "; recursing down.\n";
                return iinsert(curr.cn->array[position], key, level, parent);
            }
            // No binding at position
            else
            {
                std::cout << "No binding at position, adding to CNode.\n";
                // Create an updated version of CNode containing new key.
                unsigned int newBitMap;
                
                NodePtr cnPtr = initNodePtr(t_CNode, key);               
                NodePtr snPtr = initNodePtr(t_SNode, key);
                NodePtr inPtr = initNodePtr(t_INode, key);
                
                // Update array and bitmap.
                cnPtr.cn->copyArray(curr.cn->array);
                newBitMap = curr.cn->bmp;
                //newBitMap |= flag;
                cnPtr.cn->bmp = newBitMap;
                cnPtr.cn->addToArray(position, snPtr);
                snPtr.sn->parent = cnPtr;

                // Create new INode to point to updated CNode
                inPtr.in = new INode(t_CNode, cnPtr);
                cnPtr.cn->parentINode = *parent;
                
                // TODO: CAS on parent INode
                *parent = inPtr.in;
                return(true);
            }
            break;
        }
        case t_INode:
        {
            std::cout << "Recursing bc INode.\n";
            // Repeat operation recursively.
            return iinsert(curr.in->main, key, level+1, parent);
            break;
        }
        case t_SNode: 
        {
            // If this is a tomb node, perform a clean() operation and return.
            if(curr.sn->tomb)
            {
                if(!curr.sn->parent.isNull)
                {
                    //clean(parent);
                    std::cout << "Would be cleaning here.\n";
                }
                return true;
            }
            else
            {
                unsigned int currHash = curr.sn->key.hashCode;
                // If this is the same value, replace it with a new binding.
                if(currHash == key.hashCode && curr.sn->key.value == key.value)
                {
                    std::cout << "Collision but values are the same.\n";
                    NodePtr snPtr = initNodePtr(t_SNode, key);
                    snPtr.sn->parent = curr.sn->parent;

                    // TODO: modify so that only INodes are CAS'd
                    // Create new CNode to update this SNode's parent CNode
                    //NodePtr cnPtr = parent;
                    // Create new INode to replace this SNode's parent CNode's parent INode
                    //NodePtr inPtr = initNodePtr(t_INode, key);
                    //inPtr.in->main = parent;

                    // TODO: CAS on curr
                    curr = snPtr;
                    return(true);
                }
                // Different value but same hash prefix, so extend tree one level down with both keys.
                else
                {
                    std::cout << "Same hash prefix, extending tree.\n";
                    // Create new CNode containing current and new SNodes
                    NodePtr cn2Ptr = initNodePtr(t_CNode, key);
                    NodePtr snPtr = initNodePtr(t_SNode, key);

                    int i = calculateIndex(key, level+1, cn2Ptr.cn);
                    int j = calculateIndex(curr.sn->key, level+1, cn2Ptr.cn);
                    cn2Ptr.cn->addToArray(i, snPtr);
                    cn2Ptr.cn->addToArray(j, curr);
                    snPtr.sn->parent = cn2Ptr;
                    curr.sn->parent = cn2Ptr;

                    // Create new INode to point to new CNode
                    NodePtr in2Ptr = initNodePtr(t_INode, key);
                    in2Ptr.in = new INode(t_CNode, cn2Ptr);
                    cn2Ptr.cn->parentINode = in2Ptr.in;

                    // Create updated version of parent CNode so that it points to new INode at position SNode is moving from.
                    NodePtr cn1Ptr = initNodePtr(t_CNode, key);

                    cn1Ptr.cn->copyArray(curr.sn->parent.cn->array);
                    //int bitMap = parent.cn->bmp;
                    i = calculateIndex(curr.sn->key, level, cn1Ptr.cn);
                    cn1Ptr.cn->addToArray(j, in2Ptr);

                    // Create new INode to point to updated parent CNode
                    NodePtr in1Ptr = initNodePtr(t_INode, key);
                    in1Ptr.in = new INode(t_CNode, cn1Ptr);
                    cn1Ptr.cn->parentINode = *parent;

                    // TODO: CAS on parent CNode's parent INode
                    curr.sn->parent.cn->parentINode = in1Ptr.in;
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

bool CTrie::lookup(int val)
{
    KeyType key = KeyType(val);
    NodePtr curr;
    curr.in = root;
    curr.type = t_INode;
    // If root is null, tree is empty.
    if(curr.in == NULL)
    {
        return false;
    }
    // If INode has no main, set root bak to NULL before continuing.
    else if(curr.in->main.isNull)
    {
        // TODO: CAS on root
        root = NULL;
        return lookup(key.value);
    }
    // Read next node below INode.
    else
    {
        int found = ilookup(curr, key, -1, &root);
        if(found != RESTART)
        {
            if(found == NOTFOUND)
                return false;
        }
        else
            return lookup(key.value);
    }
    return true;
}

bool CTrie::ilookup(NodePtr curr, KeyType key, int level, INode **parent)
{
    switch(curr.type)
    {
        case t_CNode:
        {
            // Appropriate entry in array must be found.
            int index = calculateIndex(key, level, curr.cn);
            
            // Bitmap shows no binding, terminate.
            if(curr.cn->array[index].isNull)
                return NOTFOUND;
            
            // Branch is in CNode, position is used an an index into array.
            else
            {
                //position = (bitMap == 0xffffffff) ? index : foo.count();
                NodePtr temp = curr.cn->array[index];
                switch(temp.type)
                {
                    case t_INode: 
                    {
                        // Repeat operation recursively.
                        return ilookup(temp.in->main, key, level+1, &(temp.in));
                        break;
                    }
                    case t_SNode: 
                    {
                        // Return value if keys match.
                        if(temp.sn->key.value == key.value)
                            return key.value;
                        else
                            return NOTFOUND;
                        break;
                    }
                    default: 
                        return NOTFOUND;
                }
            }               
            break;
        }
        case t_SNode: 
        {
            // TODO: If this is a tomb node, clean(parent)
            if(curr.sn->tomb)
            {
                // clean
            }
            return RESTART;
            break;
        }
        case t_INode:
        {
            return ilookup(curr.in->main, key, level + 1, &(curr.in));
            break;
        }
        default: 
            return NOTFOUND;
    }
}


int main(void)
{
    std::cout << "ayyy in program.\n";
    CTrie::CTrie myTrie;
    std::cout << "myTrie is supposedly created.\n";
    myTrie.insert(1);
    std::cout << "Returned from insert(1).\n";
    myTrie.insert(12);
    std::cout << "Returned from insert(2).\n";
    myTrie.lookup(1);
    myTrie.lookup(12);

    return 0;
}