#include <stm/config.h>
#if defined(STM_CPU_SPARC)
#include <sys/types.h>
#endif

#include <iostream>
#include <alt-license/rand_r_32.h>
#include <api/api.hpp>
#include "bmconfig.hpp"

#ifdef SINGLE_SOURCE_BUILD
#include "bmharness.cpp"
#endif


#include<iostream>
#include<thread>
#include<vector>
#include<atomic>
#include<chrono>
#include<functional>
#include "rstm.hpp"

// Find appropriate index in a CNode's array using a key's hash code.
TM_CALLABLE
int CTrie::calculateIndex(KeyType key, int level)
{
    int index = (key.hashCode >> level & 0x1f);
    return index;
}

CNode *initCNode()
{
    CNode *temp = new CNode;
    temp->initArray();
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
    currRoot.in = TM_READ(root);
    currRoot.type = t_INode;
    // If root is null or its next pointer is null, then tree is empty.
    if(currRoot.in == NULL || (currRoot.in->main.isNull))
    {
        // Create new CNode that contains the new SNode with the key, and a new INode to point to it.
        NodePtr cnPtr = initNodePtr(t_CNode, key);
        NodePtr snPtr = initNodePtr(t_SNode, key);
        NodePtr inPtr = initNodePtr(t_INode, key);

        do
        {
            int i = calculateIndex(key, 0);
            cnPtr.cn->addToArray(i, snPtr);
            cnPtr.cn->array[i].sn->parent = cnPtr;

            // Initialize new INode's main to the new CNode.
            inPtr.in = (new INode(t_CNode, cnPtr));    
            
            // TODO: Compare and swap INode at root.
            root = inPtr.in;
            root->main.cn->parentINode = root;
        } while(root != inPtr.in);
        return true;    
    }
    // Root points to valid INode.
    else 
    {
        bool result = iinsert(currRoot, key, -1, &root);
        while(!result) 
        {
            return insert(val);
        }
    }
    return true;
}

bool CTrie::iinsert(NodePtr curr, KeyType key, int level, INode **parent)
{
    switch(curr.type)
    {
        case t_CNode:
        {
            // Appropriate entry in array must be found.
            int position = calculateIndex(key, level);

            // If there is a binding at the position in CNode and it's not null, repeat operation recursively.
            if(!curr.cn->array[position].isNull)
            {
                return iinsert(curr.cn->array[position], key, level, parent);
            }
            // No binding at position
            else
            {
                // Create an updated version of CNode containing new key.                
                NodePtr cnPtr = initNodePtr(t_CNode, key);               
                NodePtr snPtr = initNodePtr(t_SNode, key);
                NodePtr inPtr = initNodePtr(t_INode, key);
                
                // Update array.
                cnPtr.cn->copyArray(curr.cn->array, curr.cn->numElements);
                cnPtr.cn->addToArray(position, snPtr);

                // Update all other array entries' parent reference.
                cnPtr.cn->updateParentRef(cnPtr);
                // Create new INode to point to updated CNode
                cnPtr.cn->parentINode = *parent;
                inPtr.in = new INode(t_CNode, cnPtr);
                
                // TODO: CAS on parent INode.
                **parent = *inPtr.in;
                return(true);
            }
            break;
        }
        case t_INode:
        {
            // Repeat operation recursively.
            return iinsert(curr.in->main, key, level+1, &(curr.in));
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
                }
                return true;
            }
            else
            {
                unsigned int currHash = curr.sn->key.hashCode;
                // If this is the same value, replace it with a new binding.
                if(currHash == key.hashCode && curr.sn->key.value == key.value)
                {
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
                    // Create new CNode containing current AND new SNodes
                    NodePtr cn2Ptr = initNodePtr(t_CNode, key);
                    NodePtr snPtr = initNodePtr(t_SNode, key);
                    NodePtr oldsnPtr = curr;

                    int i = calculateIndex(key, level+1);
                    int j = calculateIndex(curr.sn->key, level+1);
                    cn2Ptr.cn->addToArray(i, snPtr);
                    cn2Ptr.cn->addToArray(j, oldsnPtr);
                    
                    // Create new INode to point to new CNode
                    NodePtr in2Ptr = initNodePtr(t_INode, key);
                    cn2Ptr.cn->parentINode = in2Ptr.in;
                    cn2Ptr.cn->updateParentRef(cn2Ptr);
                    in2Ptr.in = new INode(t_CNode, cn2Ptr);                  

                    // Create updated version of parent CNode so that it contains new INode at position SNode is moving from.
                    NodePtr cn1Ptr = initNodePtr(t_CNode, key);
                    cn1Ptr.cn->copyArray(curr.sn->parent.cn->array, curr.sn->parent.cn->numElements);
                    i = calculateIndex(curr.sn->key, level);
                    cn1Ptr.cn->addToArray(j, in2Ptr);

                    // Create new INode to point to updated parent CNode
                    NodePtr in1Ptr = initNodePtr(t_INode, key);
                    in1Ptr.cn->parentINode = *parent;
                    in1Ptr.in = new INode(t_CNode, cn1Ptr);

                    // TODO: CAS on parent CNode's parent INode
                    curr.sn->parent.cn->parentINode = in1Ptr.in;
                    return(true);
                }
            }
            break;
        }
        default: 
            return true;
    }   
}

bool CTrie::lookup(int val TM_ARG)
{
    KeyType key = KeyType(val);
    NodePtr curr = (NodePtr*)TM_ALLOC(sizeof(NodePtr));
    curr.in = TM_READ(root);
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
        TM_WRITE(root, NULL);
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

int CTrie::ilookup(NodePtr curr, KeyType key, int level, INode **parent)
{
    switch(curr.type)
    {
        case t_CNode:
        {
            // Appropriate entry in array must be found.
            int index = calculateIndex(key, level);
            
            // No binding, terminate.
            if(curr.cn->array[index].isNull)
                return NOTFOUND;
            
            // Branch is in CNode, position is used an an index into array.
            else
            {
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
        {
            //clean
            return NOTFOUND;
        }
    }
}

bool CTrie::remove(int val)
{
    KeyType key = KeyType(val);
    NodePtr curr;
    curr.in = root;
    // If root is null, tree is empty.
    if(curr.in == NULL)
        return NOTFOUND;
    else if(curr.in->main.isNull)
    {
        // TODO: CAS on root
        return remove(val);
    }
    else
    {
        int result = iremove(curr, key, -1, &root);
        if(result != RESTART)
        {
            if(result == NOTFOUND)
                return false;
            return true;
        }
        else
            remove(val);
    }
    return true;
}

int CTrie::iremove(NodePtr curr, KeyType key, int level, INode **parent)
{
    switch(curr.type)
    {
        case t_CNode: 
        {
            // Appropriate entry in array must be found.
            int position = calculateIndex(key, level);
            int ret;
            // Binding not found.
            if(curr.cn->array[position].isNull)
                return NOTFOUND;
            else
            {
                // TODO: start compression.
                NodePtr temp = curr.cn->array[position];
                return iremove(temp, key, level, parent);
                // if(ret == NOTFOUND || ret == RESTART)
                //     return ret;
                // TODO: && tombCompress() vvv
                // if((*parent) != NULL)
                //     std::cout << "contract parent" << std::endl;

                break;
            }
            
        }
        case t_INode:
        {
            return iremove(curr.in->main, key, level+1, &(curr.in));
            break;
        }
        case t_SNode: 
        {
            if(curr.sn->tomb)
            {
                if(parent != NULL)
                // clean()
                return RESTART;
            }
            // Value is found.
            if(curr.sn->key.value == key.value)
            {
                int position = calculateIndex(key, level);

                // Create updated version of CNode
                NodePtr cnPtr = initNodePtr(t_CNode, key);
                cnPtr.cn->copyArray(curr.sn->parent.cn->array, curr.sn->parent.cn->numElements);
                cnPtr.cn->removeFromArray(position);
                cnPtr.cn->parentINode = *parent;
                
                // Create new INode to point to updated CNode
                NodePtr inPtr = initNodePtr(t_INode, key);
                inPtr.in = new INode(t_CNode, cnPtr);

                // TODO: compare and swap on CNode's parent INode
                **parent = *inPtr.in;
                // If CAS, return sn.v else RESTART
                if(*parent == inPtr.in)
                    return key.value;
                else
                    return RESTART;
            }
            else
                return NOTFOUND;
            break;
        }
        default:
        {
            return NOTFOUND;
        }                       
    }
    return NOTFOUND;
}

NodePtr CTrie::toWeakTomb(NodePtr node)
{
    NodePtr temp = node;
    temp.cn->removeNullINodes();

    // If number of nodes in CNode that are not null-INodes is greater than
    // one, then there is nothing to entomb.
    if(temp.cn->numElements > 1)
        return node;
    // Single branch exists
    if(temp.cn->numElements == 1)
    {
        int i;
        for(i = 0; i < LENGTH; i++)
        {
            if(!node.cn->array[i].isNull)
            {
                NodePtr s = node.cn->array[i];
                // If the single branch below this CNode is another CNode, return a copy of current
                // CNode with null-INodes removed.
                if(s.type == t_INode && s.in->main.type == t_CNode)
                    return temp;
                // If single branch is an SNode, return it as a tomb node.
                else if(s.type == t_SNode)
                {
                    s.sn->tomb = true;
                    return s;
                }
            }         
        }
    }
    temp.isNull = true;
    return temp;
}

bool CTrie::tombCompress(INode **parent)
{
    INode *temp1 = *parent;
    NodePtr temp = temp1->main;
    if(temp.type != t_CNode)
        return false;
    NodePtr weakTomb = toWeakTomb(temp);
    if(temp.cn == weakTomb.cn)
        return false;
    
    // Create new INode to point to this weak tomb
    KeyType dummy = KeyType(0);
    NodePtr inPtr = initNodePtr(t_INode, dummy);
    
    NodeType type = weakTomb.type;
    if(type == t_CNode)
    {
        weakTomb.cn->parentINode = *parent;
        inPtr.in = new INode(t_CNode, weakTomb);
    }
    else if(type == t_SNode)
    {
        weakTomb.sn->parent.cn->parentINode = *parent;
        inPtr.in = new INode(t_SNode, weakTomb);
    }
    else
        inPtr.isNull = true;

    // TODO: CAS on parent INode
    **parent = *inPtr.in;
    if(*parent == inPtr.in)
    {
        // Parent should be contracted.
        if(weakTomb.isNull || weakTomb.type == t_SNode)
            return true;
        else
            return false;
    }
    // Continually try to entomb current CNode until success or nothing to entomb.
    else
        return tombCompress(parent);
}

/*
NodePtr toCompressed(NodePtr node)
{
    int num = node.cn->numElements;

    if(num == 1)
    {
        int i = node.cn->isTombINode();
        if(i != NOTFOUND)
            return node.cn->array[i].in->main;
    }
    NodePtr temp = node;
    temp.cn->removeNullINodes();
}

NodePtr resurrect(NodePtr node)
{

}

void clean(INode parent)
{

}

void contractParent(INode parent, unsigned int hashCode, int level)
{

}
*/

int main(void)
{
    CTrie::CTrie myTrie;
    myTrie.insert(1);
    myTrie.insert(12);
    myTrie.lookup(1);
    myTrie.lookup(12);
    myTrie.remove(1);

    return 0;
}
