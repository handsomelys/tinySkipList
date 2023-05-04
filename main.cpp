// #include <bits/stdc++.h>
#include <iostream>
#include "skiplist.h"
#define FILE_PATH "./store/dumpFile"
// using namespace std;

int main() {
    SkipList<std::string, std::string> skipList(6);
    std::cout << "--- insert  test ---" << std::endl;
    skipList.insert_element("1", " one ");
    skipList.insert_element("2", " two "); 
	skipList.insert_element("3", " three "); 
	skipList.insert_element("abc", " test 1"); 

    std::cout << "skipList size after insert_element(): " << skipList.size() << std::endl;
    std::cout << "--- search test ---" << std::endl;
    skipList.search_element("3");
    skipList.search_element("4");

    std::cout << "--- delete test ---" << std::endl;
    skipList.delete_element("3");
    skipList.search_element("3");
    std::cout << "after delete, skip list size: " << skipList.size() << std::endl;

    std::cout << "--- dump file test ---" << std::endl;
    skipList.dump_file();

    std::cout << "--- display test ---" << std::endl;
    skipList.display_list();

    std::cout << "--- clear test ---" << std::endl;
    skipList.clear();
    std::cout << "after clear, skip list size: " << skipList.size() << std::endl;

    std::cout << "--- load_file test ---" << std::endl;
    skipList.load_file();
    std::cout << "after load, skip list size: " << skipList.size() << std::endl;
    skipList.display_list();

    return 0;
}
