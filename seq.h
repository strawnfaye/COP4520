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
    struct INode *in;
    struct CNode *cn;
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

struct INode 
{
    enum NodeType type;
    std::atomic<struct NodePtr> main;		//changed

    INode(NodeType type, NodePtr inMain)
    : main(inMain)
    {
        main = inMain;
        this->type = type;   
    }
	
	bool CAS(NodePtr newVal)
	{
		NodePtr oldVal = main.load(std::memory_order_relaxed);
		return main.compare_exchange_strong(oldVal, newVal);
	}
};

struct CNode 
{
    int bmp;
    std::atomic<NodePtr>* array;		//changed

	CNode()		//changed: new constructor
	{
		bmp = 0;
		array = new std::atomic<NodePtr>[LENGTH];
	}
	
    void addToArray(int i, NodePtr node)
    {
        array[i].store(node, std::memory_order_relaxed);		//changed for atomic
    }

    NodePtr* copyArray(NodePtr* to)
    {
        int i;
        for(i = 0; i < LENGTH; i++)
        {
			NodePtr storedNode = array[i].fetch(std::memory_order_relaxed);	//changed for atomic
            to[i].store(storedNode, memory_order_relaxed);
        }
        return to;
    }
};

class CTrie 
{
    private:
    std::atomic<INode> *root;		//changed

    public:
    CTrie()
    {
        root = NULL;
    }

    int calculateIndex(KeyType key, int level, CNode *cn);
    bool insert(int val);
    bool iinsert(NodePtr curr, KeyType key, int level, NodePtr parent);
};