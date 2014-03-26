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
        // передается именно указатель на список, хотя я бы назвал его скорее указатель на последний элемент
        // после которого нужно вставлять запись
        // кстати, а можно вставлять запись во время итерации? т.е. выбрать элемент и тут же после него вставить запись?
        list_add(&p1, &mylist);
        struct list_head *tail = mylist.prev;
        list_add(&p2, tail);
        
        // можно вставить в самое начало, после самого первого элемента
        // а можно в самый конец, для этого нужно взять начало и вытащить у него prev
        // head.prev == last
        // last.next == head
        
        printf("head = %p\n", &mylist);
        printf("  p1 = %p\n", &p1);
        printf("  p2 = %p\n", &p2);
        
        printf("%p\n", &mylist);
        printf("%p\n", mylist.next);
        printf("%p\n", mylist.next->next);
        printf("%p\n", mylist.next->next->next);
        // таким образом формируется список
        
        printf("list for each\n");
        
        struct list_head *el;
        __list_for_each(el,&mylist) {
            printf("%p\n", el);
        }
    }
    
    {
        printf("\nstack\n");

        LIST_HEAD(list);
        LIST_HEAD(p1);
        LIST_HEAD(p2);
        printf("  p1 = %p\n", &p1);
        printf("  p2 = %p\n", &p2);
        list_add(&p1, &list);
        list_add(&p2, &list);
        struct list_head *el;
        __list_for_each(el,&list) {
            printf("%p\n", el);
        }
    }
    
    {
        LIST_HEAD(mlist);
        struct mystruct z = list_entry(&mlist,struct mystruct,mylist);
    }
}