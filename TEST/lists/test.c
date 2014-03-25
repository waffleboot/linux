#include <linux/list.h>

struct mystruct {
    int data;
    struct list_head mylist;
};

int main() {
    struct mystruct first = {
        .data = 10,
        .mylist = LIST_HEAD_INIT(first.mylist)
        // это макрос
    };
    struct mystruct second;
    second.data = 20;
    INIT_LIST_HEAD(&second.mylist);
    struct list_head a = LIST_HEAD_INIT(second.mylist);
    struct list_head b = LIST_HEAD_INIT(a);
    // можно создавать крутые структуры, почти деревья, где список указывает на другой список
    struct list_head *p = &a;
    INIT_LIST_HEAD(p);
    
    LIST_HEAD(a2);
    LIST_HEAD(b2);
    INIT_LIST_HEAD(&b2);

    //list_add(new,head);
    {
        LIST_HEAD(mylinkedlist);
        LIST_HEAD(e1);
        LIST_HEAD(e2);
        list_add(&e1, &mylinkedlist);
        list_add(&e2, &mylinkedlist);
    }
    
    {
        LIST_HEAD(mylist);
        LIST_HEAD(p1);
        LIST_HEAD(p2);
        list_add(&p1, &mylist);
        struct list_head *tail = mylist.prev;
        list_add(&p2, tail);
        
        // можно вставить в самое начало, после самого первого элемента
        // а можно в самый конец, для этого нужно взять начало и вытащить у него prev
        // head.prev == last
        // last.next == head
        
        printf("head = %lx\n", (unsigned long)&mylist);
        printf("  p1 = %lx\n", (unsigned long)&p1);
        printf("  p2 = %lx\n", (unsigned long)&p2);
        
        printf("%lx\n", (unsigned long)&mylist);
        printf("%lx\n", (unsigned long)mylist.next);
        printf("%lx\n", (unsigned long)mylist.next->next);
        printf("%lx\n", (unsigned long)mylist.next->next->next);
        // таким образом формируется список
    }
}