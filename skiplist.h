#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>

#define STORE_FILE "store/dumpFile"

std::mutex mtx, mtx1;
std::string delimiter = ":";


// https://czgitaccount.github.io/Database/Skiplist-CPP/
// 一个关于skiplist的解读网站
template<typename K, typename V>
class Node {
    public:
        Node() {}   // 无参构造函数是必须的吗？

        Node(K k, V v, int);

        ~Node();

        K get_key() const;  //  const就表明这俩函数只读不写

        V get_value() const;

        void set_value(V);

        Node<K, V> **forward;   //  前向指针数组 内部存前向指针 指向下一个Node

        int node_level; //指示当前这个节点的层数
    
    private:
        K key;
        V value;
};

template<typename K, typename V>
Node<K, V>::Node(const K k, const V v, int level) {
    this->key = k;
    this->value = v;
    this->node_level = level;
//  Node<K，V>*是类型 node的指针 其实加个空格就好理解了
//  forward 所在的层数是 level + 1，forward就是ppt示意图中 竖着的那块数组
    this->forward = new Node<K, V>*[level + 1]; // 这块怎么去遍历forward

    memset(this->forward, 0, sizeof(Node<K, V>*)*(level+1));
};

template<typename K, typename V>
Node<K, V>::~Node() {
    // new的时候用了[]， delete的时候也用[]
    delete []forward;
};

// 为什么两个get方法不需要使用this
template<typename K, typename V>
K Node<K, V>::get_key() const {
    return key;
};

template<typename K, typename V>
V Node<K, V>::get_value() const {
    return value;
};

template<typename K, typename V>
void Node<K, V>::set_value(V value) {
    this->value = value;
}

template <typename K, typename V>
class SkipList {
    public:
        SkipList(int);
        ~SkipList();
        int get_random_level();
        Node<K, V>* create_node(K, V, int);
        int insert_element(K, V);
        int update_element(const K, const V, bool=false);
        bool search_element(K);
        int delete_element(K);
        void dump_file();
        void load_file();
        void display_list();
        int size();
        void clear();
    private:
        void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
        bool is_valid_string(const std::string& str);

    private:
        int _max_level;

        int _skip_list_level;

        Node<K, V>* _header;

        std::ofstream _file_writer;
        std::ifstream _file_reader;

        int _element_count;
};

template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
    Node<K, V>* n = new Node<K, V>(k, v, level);
    return n;
}

template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {
    mtx.lock();
    Node<K, V>* current = this->_header;

    // update 是一个指针数组，数组内存放指针，指向node结点，其索引代表层
    Node<K, V>* update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));

    // 从高level开始往下遍历 找到要插入的那个level
    for(int i = _skip_list_level; i>=0; i--) {
        // 只要当前节点非空，且key小于目标，就会向后遍历
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];  //节点向后移动
        }
        update[i] = current;    //update[i]记录当前层最后符合要求的节点
    }

    // 遍历到level0说明到达最底层了，forward[0]指向的就是跳表的下一个邻近节点
    current = current->forward[0];
    // 此时 current->get_key() >= key !!

    // 1. 插入元素已经存在
    if (current != NULL && current->get_key() == key) {
        std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();
        return 1;
    }

    // 2. 当前current不存在 或者current->get_key > key
    if (current == NULL || current->get_key() != key) {
        //  随机生成层的高度 也就是forward的大小
        int random_level = get_random_level();

        // 如果新添加的节点level大于当前跳表的level 则更新update
        // 将原本[_skip_list_level, random_level]范围内的NULL改为_header
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level+1; i < random_level+1; i++) {
                update[i] = _header;
            }
            _skip_list_level = random_level;    // 最后更新跳表的level
        }

        // 创建节点 并进行插入操作
        Node<K, V>* inserted_node = create_node(key, value, random_level);

        // 该操作等价于：
        // new_node->next = pre_node->next;
        // pre_node->next = new_node;
        // 只不过是逐层进行
        for (int i = 0; i <= random_level; i++) {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }

        std::cout << "Successfully inserted key: " << key << ", value: " << value << std::endl;
        _element_count++;   // 更新结点数
    }
    mtx.unlock();
    return 0;
}

template<typename K, typename V>
void SkipList<K, V>::display_list() {
    std::cout << "\n*****Skip List*****"<<"\n";
    for (int i = 0; i <= _skip_list_level; i++) {
        Node<K, V>* node = this->_header->forward[i];
        std::cout << "Level " << i << ": ";
        while (node != NULL) {
            std::cout << node->get_key() << ":" << node->get_value() << ";";
            node = node->forward[i];
        } 
        std::cout << std::endl;
    }
}

template<typename K, typename V>
// 这个dump怎么实现的？
void SkipList<K, V>::dump_file() {
    std::cout << "dump_file--------------" << std::endl;
    _file_writer.open(STORE_FILE);
    Node<K, V>* node = this->_header->forward[0];
    
    while (node != NULL) {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];
    }

    _file_writer.flush();
    _file_writer.close();
    return ;
}

template<typename K, typename V>
void SkipList<K, V>::load_file() {
    // 其实看到这里就懂了
    // dump_file其实保存里里面的键值对，而没有保存他们的level
    // 读取他们键值对 然后调用insert方法去重构跳表
    // 读取->重建的跳表 可能和原本的不一样？因为有randomlevel的存在
    _file_reader.open(STORE_FILE);
    std::cout << "load_file-----------------"<<std::endl;
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();
    while (getline(_file_reader, line)) {
        get_key_value_from_string(line, key, value);
        if (key->empty() || value->empty()) {
            continue;
        }
        insert_element(*key, *value);
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    _file_reader.close();
}

template<typename K, typename V>
int SkipList<K, V>::size() {
    return _element_count;
}


/*
指针传递参数本质上是值传递的方式，它所传递的是一个地址值。值传递过程中，被调函数的形式参数作为被调函数的局部变量处理，即在栈中开辟了内存空间以存放由主调函数放进来的实参的值，从而成为了实参的一个副本。值传递的特点是被调函数对形式参数的任何操作都是作为局部变量进行，不会影响主调函数的实参变量的值。（这里是在说实参指针本身的地址值不会变）

而在引用传递过程中，被调函数的形式参数虽然也作为局部变量在栈中开辟了内存空间，但是这时存放的是由主调函数放进来的实参变量的地址。被调函数对形参的任何操作都被处理成间接寻址，即通过栈中存放的地址访问主调函数中的实参变量。正因为如此，被调函数对形参做的任何操作都影响了主调函数中的实参变量。
*/
template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {
    if (!is_valid_string(str)) {
        return ;
    }
    *key = str.substr(0, str.find(delimiter));
    *value = str.substr(str.find(delimiter)+1, str.length());
}

template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {
    if (str.empty()) {
        return false;
    }
    if (str.find(delimiter) == std::string::npos) {
        return false;
    }
    return true;
}

template<typename K, typename V>
int SkipList<K, V>::delete_element(K key) {
    mtx.lock();
    Node<K, V>* current = this->_header;
    Node<K, V>* update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));

    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    current = current->forward[0];
    //  1. 非空，且key为目标值
    if (current != NULL && current->get_key() == key) {
        //  从底层开始删除 update->forward指向的节点，即目标节点
        for (int i = 0; i <= _skip_list_level; i++) {
            //  如果update[i]已经不指向current 说明i的上层也不会指向current？？
            //  也说明被删除节点层高i - 1 直接退出循环
            if (update[i]->forward[i] != current)
                break;
            update[i]->forward[i] = current->forward[i];
        }

        //  因为可能删除的元素它的层数恰好是当前跳表的最大层数
        //  所以此时需要重新确定_skip_list_level，通过头节点判断
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0) {
            _skip_list_level--;
        }

        std::cout << "Successfully deleted key " << key << std::endl;
        _element_count--;
        mtx.unlock();
        return 0;
    } else {
        // 增加了 key不存在的时候的情况
        std::cout << key << " is not exist, please check your input !\n";
        mtx.unlock();
        return -1;
    }
}

template<typename K, typename V>
bool SkipList<K, V>::search_element(K key) {
    std::cout << "search_element------------------------" << std::endl;
    Node<K, V>* current = _header;

    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    current = current->forward[0];

    if (current and current->get_key() == key) {
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    std::cout << "Not found key: " << key << std::endl;
    return false;
}

template<typename K, typename V>
SkipList<K, V>::SkipList(int max_level) {
    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);
};

template<typename K, typename V>
SkipList<K, V>::~SkipList() {
    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }
    delete _header;
}

template<typename K, typename V>
int SkipList<K, V>::get_random_level() {
    int k = 1;
    while (rand() % 2) {
        k++;
    }
    k = (k < _max_level)?k:_max_level;
    return k;
};

// 增加一个update方法：
template<typename K, typename V>
int SkipList<K, V>::update_element(const K key, const V value, bool flag) {
    mtx1.lock();
    Node<K, V>* current = this->_header;
    Node<K, V>* update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));

    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    current = current->forward[0];

    if (current && current->get_key() == key) {
        std::cout << "key: " << key << ", exists " << std::endl;
        std::cout << "old value : " << current->get_value() << "--> ";
        current->set_value(value);
        std::cout << "new value : " << current->get_value() << std::endl;
        mtx1.unlock();
        return 1;
    }

    if (flag) {
        SkipList<K, V>::insert_element(key, value); 
        // 因为这里需要调用insert insert又需要锁 所以放了两重锁？
        // 那这两个锁不会冲突吗？
        mtx1.unlock();
        return 0;
    }
    else {
        std::cout << key << " is not exist, please check your input! \n" << std::endl;
        mtx1.unlock();
        return -1;
    }
}

template<typename K, typename V>
void SkipList<K, V>::clear() {
    std::cout << "clear..." << std::endl;
    Node<K, V>* node = this->_header->forward[0];
}