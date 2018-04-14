#include<iostream>
#include<thread>
#include<vector>
#include<atomic>
#include<chrono>
#include<functional>

#define LENGTH 32
#define W 5
#define RESTART INT_MIN
#define NOTFOUND INT_MAX

const unsigned int SK5 = 0x55555555, SK3 = 0x33333333;
const unsigned int  SKF0 = 0xF0F0F0F, SKFF = 0xFF00FF;

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
    int hashCode;

    KeyType(T value)
    {
        std::hash<T> hash;
        this->value = value;
        this->hashCode = hash(value);
    }
};

template <class T>
struct SNode 
{
    KeyType<T> key;
    NodeType type;
    bool tomb;

    SNode(T key, NodeType type, bool tomb)
    {
        this->type = type;
        this->key = key;
        this->tomb = tomb;
    }
};

template <class T>
struct CNode 
{
    int bmp;
    T array[LENGTH];

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

    // Uses bitwise operations and popcount emulation to find appropriate index in 
    // a CNode's array through its bitmap.
    int calculateIndex(KeyType<T> key, int level, CNode<T> cn)
    {
        int index = (key->hashCode >> level & 0x1f);
        int bitMap = cn->bmp;
        int flag = 1 << index;
        int mask = flag - 1;
        std::bitset<LENGTH> foo(bitMap & mask);
        return foo.count();
    }

    int calculatePrefix(int hash, int k, int p)
    {
        return(((1 << k) - 1) & (hash >> (p - 1)));
    }

    bool insert(KeyType<T> key)
    {
        INode<T> currRoot = root;
        // If root is null or its next pointer is null, then tree is empty.
        if(currRoot == NULL || currRoot->main == NULL)
        {
            // Create new CNode that contains the new SNode with the key, and a new INode to point to it.
            CNode<T> cn;
            SNode<T> sn = new SNode<T>(key, t_SNode, false);
            INode<T> in;
            do
            {
                int i = calculateIndex(key, 0, currRoot->main);
                cn->array[i] = sn;
                // Initialize new INode's main to the new CNode.
                in->main = cn;
                // TODO: Compare and swap INode at root.
                root = in;
            } while(root != in);           
        }
        // Root points to INode.
        else if(!iinsert(currRoot, key, 0, NULL))
        {
            insert(key);
        }
    }

    bool iinsert(INode<T> curr, KeyType<T> key, int level, T parent)
    {
        switch(curr->type)
        {
            case t_CNode:
            {
                // Appropriate entry in array must be found by calculating position in bitmap.
                int index = (key->hashCode >> level & 0x1f);
                unsigned int bitMap = curr->bmp;
                int flag = 1 << index;
                int mask = flag - 1;
                std::bitset<LENGTH> foo(bitMap & mask);
                int position = foo.count();

                // If there is a binding at the position in CNode and it's not null, repeat operation recursively.
                if((bitMap & flag) != 0)
                {
                    return iinsert(curr->array[position], key, level + W, curr);
                }
                // No binding at position
                else
                {
                    // Create an updated version of CNode containing new key.
                    T *temp;
                    int newBitMap;
                    CNode<T> cn;
                    SNode<T> sn = new SNode<T>(key, t_SNode, false);
                    do
                    {
                        temp = curr->array;
                        newBitMap = curr->bmp;
                        cn = new CNode<T>(newBitMap, temp);
                        cn.newBitMap & flag = 1;
                        cn.array[position] = sn;
                        // TODO: CAS on curr->main
                        curr= cn;
                    } while(curr != cn);
                    return(curr == cn);        
                }
                break;
            }
            case t_INode:
            {
                // Repeat operation recursively.
                return iinsert(curr->main, key, level + W, curr);
                break;
            }
            case t_SNode: 
            {
                // If this is a tomb node, perform a clean() operation and return.
                if(curr->tomb)
                {
                    if(parent != NULL)
                        clean(parent);
                    return true;
                }
                else
                {
                    T currHash = curr->key.hashCode;
                    // If this is the same value, replace it with a new binding.
                    if(currHash == key.hashCode && curr->key.value == key.value)
                    {
                        SNode<T> sn = new SNode<T>(key, t_SNode, false);
                        // TODO: CAS on curr
                        curr = sn;
                        return(curr == sn);
                    }
                    // Different value but same hash prefix, so extend tree one level down with both keys.
                    else
                    {
                        // Create new CNode containing SNode with this SNode and the new key
                        CNode<T> cn2;
                        SNode<T> sn = new SNode<T>(key, t_SNode, false);
                        int i = calculateIndex(key, level, cn2);
                        int j = calculateIndex(curr->key, level, cn2);
                        cn2->array[i] = sn;
                        cn2->array[j] = curr;
                        // Create new INode to point to new CNode
                        INode<T> in;
                        in->main = cn2;
                        // Create updated version of parent CNode so that it points to new INode at position SNode is moving from.
                        CNode<T> cn1;
                        T *tempArr = parent->array;
                        int bitMap = parent->bmp;
                        i = calculateIndex(curr->key, level, cn1);
                        cn1->array[i]->in;
                        // TODO: CAS on parent CNode
                        parent = cn1;
                        return(parent == cn1);
                    }
                }
                break;
            }
            default: 
                return true;
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

    void toCompressed(CNode<T> cn)
    {
        
    }

    void toWeakTombed(CNode<T> cn)
    {

    }

    void clean(INode<T> i)
    {

    }

    void tombCompress(INode<T> i)
    {

    }

    void contractParent(INode<T> parent, INode<T> i, T hashCode, int level)
    {
        
    }

    // Return number of bits set to "1" in a given integer bit map.
    int popCount(int bitMap)
    {
        bitMap -= ((bitMap>>1) & SK5);
        bitMap = (bitMap & SK3) + ((bitMap >> 2) & SK3);
        bitMap = (bitMap & SKF0) + ((bitMap >> 4) & SKF0);
        bitMap += bitMap >> 8;
        return ((bitMap + (bitMap >> 16)) & 0x3F);
    }
    
};