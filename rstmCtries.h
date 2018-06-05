#include<iostream>
#include<thread>
#include<vector>
#include<atomic>
#include<chrono>
#include<functional>
#include<cmath>

#define LENGTH 32
#define W 5
#define RESTART INT_MIN
#define NOTFOUND INT_MAX
#define HASHY 2654435761

enum NodeType
{
    t_INode,
    t_CNode,
    t_SNode
};

struct NodePtr
{
    enum NodeType type;
    struct SNode *sn;
    struct INode* in;
    struct CNode *cn;
    bool isNull;
};

struct KeyType 
{
    int value;
    unsigned int hashCode;

    KeyType(char value)
    {
        this->value = value;
        this->hashCode = (value * HASHY) % (int) (std::pow(2, LENGTH));
    }
};

struct SNode
{
    struct KeyType key;
    enum NodeType type;
    bool tomb;
    NodePtr parent; 

    SNode(KeyType inKey, NodeType type, bool tomb)
    : key(inKey) 
    {
        key = inKey;
        this->type = type;
        this->tomb = tomb;
    }
};

struct INode 
{
    enum NodeType type;
    struct NodePtr main;

    INode(NodeType type, NodePtr inMain)
    : main(inMain)
    {
        main = inMain;
        this->type = type;   
    }

};

struct CNode 
{
    std::atomic<INode> *parentINode;
    NodePtr array[LENGTH];
    int numElements;

    void initArray()
    {
        for(int i = 0; i < LENGTH; i ++)
            array[i].isNull = true;
        numElements = 0;
    }

    void addToArray(int i, NodePtr node)
    {
        array[i] = node;
        numElements++;
    }

    void removeFromArray(int i)
    {
        array[i].isNull = true;
        numElements--;
    }

    void copyArray(NodePtr *from, int num)
    {
        int i;
        for(i = 0; i < LENGTH; i++)
            array[i] = TM_READ(from[i]);
        numElements = num;
    }  

    void updateParentRef(NodePtr newParent)
    {
        int i;
        for(i = 0; i < LENGTH; i++)
        {
            if(array[i].type == t_SNode)
                array[i].sn->parent = newParent;
        }
    }

    void removeNullINodes()
    {
        int i;
        for(i = 0; i < LENGTH; i++)
        {
            if(array[i].type == t_INode && array[i].in->main.isNull)
            {
                array[i].isNull = true;
                numElements--;
            }
        }
    }

    int isTombINode()
    {
        int i;
        for(i = 0; i < LENGTH; i++)
        {
            if(array[i].type == t_INode && array[i].in->main.isNull)
                return i;
        }
        return NOTFOUND;
    }
};

class CTrie 
{
    private:
    INode *root;

    public:
    CTrie()
    {
        root = NULL;
    }

    int calculateIndex(KeyType key, int level);
    bool insert(int val);
    bool iinsert(NodePtr curr, KeyType key, int level, INode **parent);
    bool lookup(int val);
    int ilookup(NodePtr curr, KeyType key, int level, INode **parent);
    bool remove(int val);
    int iremove(NodePtr curr, KeyType key, int level, INode **parent);
    NodePtr toWeakTomb(NodePtr node);
    bool tombCompress(INode **parent);
};