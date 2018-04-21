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

// A Generic "node" object that holds pointers to
// each type of node, the type of the node it's actually holding,
// and a null marker.
struct NodePtr
{
    enum NodeType type;
    std::atomic<struct SNode> *sn;
    std::atomic<struct INode> *in;
    std::atomic<struct CNode> *cn;
    bool isNull;
};

struct KeyType 
{
    int value;
    int hashCode;

    KeyType(int value)
    {
        this->value = value;
        this->hashCode = (value * HASHY) % (int) (std::pow(2, LENGTH));
    }
};

// Singleton node.
struct SNode
{
    struct KeyType key;
    enum NodeType type;
    bool tomb;

    SNode(KeyType inKey, NodeType type, bool tomb)
    : key(inKey) 
    {
        key = inKey;
        this->type = type;
        this->tomb = tomb;
    }
};

// Indirection node.
struct INode 
{
    enum NodeType type;
    struct NodePtr main;

    INode(NodeType type, NodePtr inMain)
    {
        main = inMain;
        this->type = type;
    }
};

// CTrie node - contains array of pointers to both
// SNodes and INodes, and a bitmap for efficient hashing.
struct CNode 
{
    unsigned int bmp;
    NodePtr array[LENGTH];

    void initArray()
    {
        for(int i = 0; i < LENGTH; i++)
        {
            array[i].isNull = true;
        }
    }

    void addToArray(int i, NodePtr node)
    {
        array[i] = node;
    }

    NodePtr *copyArray(NodePtr *to)
    {
        int i;
        for(i = 0; i < LENGTH; i++)
        {
            to[i] = array[i];
        }
        return to;
    }
};

// Lock-Free, Concurrent, Resizeable Hash Array Mapped Trie.
class CTrie 
{
    private:
    std::atomic<INode> *root;  

    public:
    CTrie()
    {
        root = NULL;
    }

    int calculateIndex(KeyType key, int level, CNode *cn);
    bool insert(int val);
    bool iinsert(NodePtr curr, KeyType key, int level, NodePtr parent);
    bool lookup(int val);
    int ilookup(NodePtr curr, KeyType key, int level, NodePtr parent);

};