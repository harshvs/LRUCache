#include <string>
#include <iostream>
#include "LRUCache.h"

using namespace std;

class MyData {
public:
    MyData() {}
    int X;
    string Y;
};

int main() {
    LRUCache<string, MyData> cache(20, 0.4);
    for(int i =0; i<10; i++) {
        MyData data;
        data.X = i;
        data.Y = to_string(i);
        cache.Put(to_string(i), data);
    }

    for(int i =0; i<30; i++) {
        cout<<i<<" - "<<cache.Exists(to_string(i))<<endl;
        if(cache.Exists(to_string(i))) {
            cout<<"\t=> "<<cache.Get(to_string(i)).X<<endl;
        }
    }

    for(int i =11; i<30; i++) {
        MyData data;
        data.X = i;
        data.Y = to_string(i);
        cout<<"Adding: "<<i<<endl;
        cache.Put(to_string(i), data);
    }

    return 0;
}
