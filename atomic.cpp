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
    unsigned int bitMap = cn->bmp;
    int flag = 1 << index;
    int mask = flag - 1;
    std::bitset<LENGTH> foo(bitMap & mask);
    return foo.count();
}

std::atomic<CNode> *initCNode()
{
    std::atomic<CNode> *cn = new std::atomic<CNode>;
    CNode temp = cn->load();
    temp.initArray();
    temp.bmp = 0;  
    cn->store(temp);
    return cn;
}

std::atomic<SNode> *initSNode(KeyType key)
{
    std::atomic<SNode> *sn = new std::atomic<SNode>;
    SNode temp = sn->load();
    temp = SNode(key, t_SNode, false);
    sn->store(temp);
    return sn;
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
    // If root is null or its next pointer is null, then tree is empty.
    if(currRoot.in == NULL || (currRoot.in->load().main.isNull))
    {
        // Create new CNode that contains the new SNode with the key, and a new INode to point to it.
        NodePtr cnPtr = initNodePtr(t_CNode, key);
        NodePtr snPtr = initNodePtr(t_SNode, key);
        NodePtr inPtr = initNodePtr(t_INode, key);

        do
        {
            // Calculate index into array, add key to array and update bit map.
            int index = (key.hashCode >> 0 & 0x1f);
            unsigned int bitMap = 0;
            int flag = 1 << index;
            int mask = flag - 1;
            std::bitset<LENGTH> foo(bitMap & mask);
            int i = foo.count();

            // Have to create temporary non-atomic objects to update nodes, then store into atomic nodes.
            CNode ctemp = cnPtr.cn->load();
            ctemp.addToArray(i, snPtr);
            ctemp.bmp |= flag;
            cnPtr.cn->store(ctemp);

            // Initialize new INode's main to the new CNode.
            INode itemp = inPtr.in->load();
            itemp = INode(t_CNode, cnPtr);
            inPtr.in->store(itemp);
            // Compare and swap INode at root.
        } while(!compare_exchange_strong(root, &currRoot.in, inPtr.in->load()));
         return true;
    }
    // Root points to valid INode.
    else 
    {
        while(!iinsert(currRoot, key, -5, currRoot))
            return insert(val);
    }
    return true;
}

bool CTrie::iinsert(NodePtr curr, KeyType key, int level, NodePtr parent)
{
    switch(curr.type)
    {
        case t_CNode:
        {
            // Appropriate entry in array must be found by calculating position in bitmap.
            int index = (key.hashCode >> level & 0x1f);
            unsigned int bitMap = curr.cn->load().bmp;
            int flag = 1 << index;
            int mask = flag - 1;
            std::bitset<LENGTH> foo(bitMap & mask);
            int position = foo.count();

            // If there is a binding at the position in CNode and it's not null, repeat operation recursively.
            // TODO: fix  to (bitMap & flag ) != 0
            if(!curr.cn->load().array[position].isNull)
            {
                return iinsert(curr.cn->load().array[position], key, level + W, curr);
            }
            // No binding at position
            else
            {
                // Create an updated version of CNode containing new key.
                unsigned int newBitMap;
                
                NodePtr cnPtr = initNodePtr(t_CNode, key);
                NodePtr snPtr = initNodePtr(t_SNode, key);
                NodePtr inPtr = initNodePtr(t_INode, key);
                
                // Update array and bitmap.
                CNode ctemp = cnPtr.cn->load();
                ctemp.copyArray(curr.cn->load().array);
                newBitMap = curr.cn->load().bmp;
                newBitMap |= flag;
                ctemp.bmp = newBitMap;
                ctemp.addToArray(position, snPtr);
                cnPtr.cn->store(ctemp);
                
                // Create new INode to point to updated CNode
                INode itemp = inPtr.in->load();
                itemp = INode(t_CNode, cnPtr);
                inPtr.in->store(itemp);
                // TODO: CAS on parent INode
                INode ptemp = parent.in->load();            
                return(parent.in->compare_exchange_strong(ptemp, inPtr.in));        
            }
            break;
        }
        case t_INode:
        {
            // Repeat operation recursively.
            return iinsert(curr.in->load().main, key, level + W, curr);
            break;
        }
        case t_SNode: 
        {
            // If this is a tomb node, perform a clean() operation and return.
            if(curr.sn->load().tomb)
            {
                if(!parent.isNull)
                {
                    //clean(parent);
                }
                return true;
            }
            else
            {
                int currHash = curr.sn->load().key.hashCode;
                // If this is the same value, replace it with a new binding.
                if(currHash == key.hashCode && curr.sn->load().key.value == key.value)
                {
                    NodePtr snPtr = initNodePtr(t_SNode, key);
                    // CAS on curr
                    SNode stemp = curr.sn->load();
                    return(curr.sn->compare_exchange_strong(stemp, snPtr.sn));
                }
                // Different value but same hash prefix, so extend tree one level down with both keys.
                else
                {
                    // Create new CNode containing current and new SNodes
                    NodePtr cn2Ptr = initNodePtr(t_CNode, key);
                    NodePtr snPtr = initNodePtr(t_SNode, key);
                    snPtr.type = t_SNode;
                    snPtr.isNull = false;
                    cn2Ptr.sn = new SNode(key, t_SNode, false);

                    int i = calculateIndex(key, level + W, cn2Ptr.cn);
                    int j = calculateIndex(curr.sn->key, level + W, cn2Ptr.cn);
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
                    cn1Ptr.cn->initArray();

                    NodePtr tempArr[32] = {};
                    parent.cn->copyArray(tempArr);
                    int bitMap = parent.cn->bmp;
                    i = calculateIndex(curr.sn->key, level, cn1Ptr.cn);
                    cn1Ptr.cn->addToArray(j, inPtr);
                    // TODO: CAS on parent CNode
                    parent = cn1Ptr;
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
        return false;
    // If INode points to null, set root back to NULL before continuing.
    else if(curr.in->main.isNull)
    {
        // TODO: CAS on root (to NULL)
        root = NULL;
        return lookup(key.value);
    }
    // Read next node below INode.
    else
    {
        int found = ilookup(curr, key, 0 , curr);
        if(found != RESTART)
        {
            if(found == NOTFOUND)
                return false;
            return true;
        }       
        else
            return lookup(key.value);
    }
}

int CTrie::ilookup(NodePtr curr, KeyType key, int level, NodePtr parent)
{
    switch(curr.type)
    {
        case t_CNode:
        {
            // Appropriate entry in array must be found.
            int index = (key.hashCode >> level & 0x1f);
            int bitMap = curr.cn->bmp;
            int flag = 1 << index;
            int mask = flag - 1;
            int position;
            std::bitset<LENGTH> foo(bitMap & mask);
            
            // Bitmap shows no binding, terminate.
            if((bitMap & flag) == 0)
                return NOTFOUND;
            
            // Branch is in CNode, position is used an an index into array.
            else
            {
                position = (bitMap == 0xffffffff) ? index : foo.count();
                NodePtr temp = curr.cn->array[position];
                switch(temp.type)
                {
                    case t_INode: 
                    {
                        // Repeat operation recursively.
                        return ilookup(temp.in->main, key, level + W, curr);
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
    myTrie.insert(45);
    std::cout << "Returned from insert(2).\n";
    myTrie.lookup(1);
    myTrie.lookup(2);

    return 0;
}

