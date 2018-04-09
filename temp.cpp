#include<iostream>
#include<thread>
#include<vector>
#include<atomic>
#include<chrono>

#define LENGTH 32
#define W 5
#define RESTART INT_MIN
#define NOTFOUND INT_MAX

enum NodeType
{
    t_INode,
    t_CNode,
    t_SNode
};

template <class T> 
struct KeyType 
{
    T value;
    T hashCode;

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

template <class T>
struct SNode 
{
    T value;
    KeyType<T> key;
    NodeType type;
    bool tomb;

    SNode(T val, T key, NodeType type, bool tomb)
    {
        this->value = val;
        this->type = type;
        this->key = key;
        this->tomb = tomb;
    }
};

template <class T>
struct CNode 
{
    int bmp;
    T array[];

    CNode(int bmp, T *array)
    {
        this->bmp = bmp;
        this->array = array;
    }
};

template <class T>
struct INode 
{
    NodeType type;
    T main;

    INode(NodeType type, T main)
    {
        this->type = type;
        this->main = main;       
    }

};

template <class T>
class CTrie 
{
    private:
    INode<T> *root;

    bool insert(KeyType<T> key, T value)
    {
        INode<T> currRoot = root;
        // If root is null or its next pointer is null, then tree is empty.
        if(currRoot == NULL || currRoot->main == NULL)
        {
            // TODO: replace root reference with new CNode with appropriate key.
            // scn = CNode(Snode(k, v, weird symbol))
            // nr = INode(scn)
            // if !CAS(root, r, nr)
            //      insert(k ,v)
        }
        // Root points to INode.
        else if(!iinsert(currRoot, key, value, 0 , NULL))
        {
            insert(key, value);
        }
    }

    bool iinsert(INode<T> curr, KeyType<T> key, T value, int level, T parent)
    {
        switch(curr->type)
        {
            case t_CNode:
            {
                // Appropriate entry in array must be found by calculating position in bitmap.
                int index = (key->hashCode >> level & 0x1f);
                int bitMap = curr->main.bmp;
                int flag = 1 << index;
                int mask = flag - 1;
                std::bitset<LENGTH> foo(bitMap & mask);
                int position = foo.count();

                // If there is a binding at the position and it's not null, insert below.
                if((bitMap & flag) != 0)
                {
                    switch(curr->main.array[position].type)
                    {
                        case t_INode: 
                        {
                            // Repeat operation recursively.
                            return iinsert(curr->main, key, value, level + W, curr);
                            break;
                        }
                        case t_SNode: 
                        {
                            // TODO: lookup to compare keys and return binding if they are the same.
                            // If keys are the same, replace old binding.
                            // Else, extend trie below CNode.
                            break;
                        }
                        default: 
                            return false;
                    }
                }
                // Branch is not in CNode
                else
                {
                    // TODO: create updated copy of current CNode with new key.
                    // nsn = SNode(k, v, null)
                    // narr = cn.array.inserted(pos, nsn)
                    // ncn = CNode(narr, bmp | flag)
                    // return CAS(curr.main, cn, ncn)
                }
                break;
            }
            case t_SNode: 
            {
                // TODO: clean
                break;
            }
            default: 
                return false;
        }
    }

    SNode<T> lookup(KeyType<T> key)
    {
        INode<T> curr = root;
        // If root is null, tree is empty.
        if(curr == NULL)
            return NULL;
        // If INode points to null, set root back to NULL before continuing.
        else if(curr->main == NULL)
        {
            CAS(root, curr, NULL);
            return lookup(key);
        }
        // Read next node below INode.
        else
        {
            T found = ilookup(curr, key, 0 , NULL);
            if(found != RESTART)
                return found;
            else
                return lookup(key);
        }
    }

    T ilookup(INode<T> curr, KeyType<T> key, int level, INode<T> parent)
    {
        switch(curr.type)
        {
            case t_CNode:
            {
                // Appropriate entry in array must be found.
                int index = (key->hashCode >> level & 0x1f);
                int bitMap = curr->main.bmp;
                int flag = 1 << index;
                int mask = flag - 1;
                int position;
                std::bitset<LENGTH> foo(bitMap & mask);
                // Bitmap shows no binding, terminate.
                if((bitMap & flag) == 0)
                    return NULL;
                // Branch is in CNode, position is used an an index into array.
                else
                {
                    position = (bitMap == 0xffffffff) ? index : foo.count();
                    switch(curr->main.array[position].type)
                    {
                        case t_INode: 
                        {
                            // Repeat operation recursively.
                            return ilookup(curr->main.array[position].main, key, level + W, curr);
                            break;
                        }
                        case t_SNode: 
                        {
                            // Return value if keys match.
                            if(curr->main.array[position].main->key.value == key.value)
                                return curr->main.array[position].main.value;
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
                return RESTART;
                break;
            }
            default: 
                return NOTFOUND;
        }
    }

    T remove(KeyType<T> key)
    {
        INode<T> curr = root;
        // If root is null, tree is empty.
        if(curr == NULL)
            return NOTFOUND;
        // If root's main is null, tree should be empty.
        else if(curr->main == NULL)
        {
            CAS(root, curr, NULL);
            return remove(key);
        }
        else
        {
            T result = iremove(curr, key, 0 , NULL);
            if(result != RESTART)
                return result;
            else remove(key);
        }
    }

    T iremove(INode<T> curr, KeyType<T> key, int level, INode<T> parent)
    {
        switch(curr->type)
        {
            case t_CNode: 
            {
                // Appropriate entry in array must be found.
                int index = (key->hashCode >> level & 0x1f);
                int bitMap = curr->main.bmp;
                int flag = 1 << index;
                int mask = flag - 1;
                int position;
                T result;
                // Binding not found.
                if((bitMap & flag) == 0)
                    return NOTFOUND;
                else
                {
                    std::bitset<LENGTH> foo(bitMap & mask);
                    position = foo.count();
                    // TODO: start compression.
                    break;
                }
                
            }
            case t_SNode:
            {
                break;
            }
            default: 
                return NOTFOUND;
        }
    }
};