#ifndef _LINUX_LIST_H
#define _LINUX_LIST_H

#ifdef __KERNEL__

#include <linux/stddef.h>
#include <linux/poison.h>
#include <linux/prefetch.h>
#include <asm/system.h>

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

/*
 берем элемент, который собираемся сохранять в списке
 добавляем в него list_head
 это структура из двух полей: ссылка на предыдущий такой же и на следующий такой же
 указатели
 */
struct list_head {
	struct list_head *next, *prev;
};

/*
 да, list_head это два указателя
 поэтому если вызвать LIST_HEAD_INIT(zoo)
 это развернется в
 {&(zoo),&(zoo)}
 но если передать LIST_HEAD_INIT(first.mylist)
 {&(first.mylist), &(first.mylist) }
 т.е. структура получит два указателя на одно и тоже
 это макрос, который сразу заполняет указатели
 видимо используется при первоначальном создании
 внутри кода использовать не получилось, только при определении самого элемента
 */
#define LIST_HEAD_INIT(name) { &(name), &(name) }

/*
 описалово, создает структуру в виде переменной
 struct list_head name = { &name, &name };
 прикольная вещь, заголовок списка содержит только указатели
 и вовсе нет нужны создавать весь элемент, куда входит этот список
 воистину, list head, только голова списка
 */
#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

/*
 а это функция
 только выше передается имя и по нему инициализируется указатель
 дело в том, что LIST_HEAD_INIT можно вызывать когда имя определено, четко указано и можно получить адрес
 INIT_LIST_HEAD так не пройдет, потому что адрес переменной это адрес переменной, а не то, что за ней стоит
 это тоже инициализация, только передается уже определенный объект и инициализируется
 видимо используется когда нужно инициализировать список динамически, когда он создается динамически
 например tss.files, т.е. внутри каждого процесса нужно завести список файлов, придется дергать эту процеруру
 а вообще надо посмотреть по коду как это используется и действительно ли только в коде
 */
static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
/*
 все что делает эта фунция это вставляет элемент между двумя другими
 и это очень интересно
 эта функция вставляет элемент между двумя другими
 и prev/next скоррелированы, т.е. взаимосвязаны и prev.next = next и next.prev = prev
 иначе потеряем элемент, сам он будет в памяти, а вот в списке его не будет
 интересно и что в этом случае делать? является ли это утечкой памяти?
 ну, в общем смысле да, потому что память это не то, что на железе, а то что описано
 вот есть список свободной памяти, вот столько значит ее и есть и даже если элемент будет потерян
 это означает утечку памяти
 эта функция только для использования внутри
 при этом используя CONFIG_DEBUG_LIST можно дебажить и написать свою функцию
 */
#ifndef CONFIG_DEBUG_LIST
static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	// интересно, а порядок важен?
	// и кстати, как тут с SMP?
	// важная штука оказывается, именно с настройки prev и должно все начинаться
	// дело в том, что есть list_empty_careful функцию и она в конце проверяет что у head.prev не поменялось значения
	// если список пустой изначально и мы добавляем в него элемент, то сначала меняем prev у head фактически
	// head.next же меняется последним тут, а проверяется первым
	// может быть это признак того, что другой проц поменял список
	// безопасно вызывать list_empty_careful на не пустом, но там надо смотреть что с удалением
	// может быть такая же процедура
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}
#else
// прикольно, можно оказывается дебажить добавление в списки
extern void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next);
#endif

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
/*
 очень замечательная функция, добавляет элемент после head
 т.е. реализует стек, новый элемент вставляется сразу после head
 так можно организовать стек
 head это не очень хороше название, я бы лучше назвал last, потому что после него вставляется
 функция удобна тем что можно взять заголовок и добавить перед ним, фактически в самый конец
 ведь head.prev это указатель на хвост списка
 и передав хвост списка мы тем самым добавим элемент в самый конец
 название не стоило менять, это действительно голова списка, а не просто последний элемент после которого вставляем
 у нас нет последнего элемента, у нас есть только голова списка и вообще говоря head это не совсем даже голова
 потому что из него можно пойти в разные стороны и еще он не хранит дополнительную информацию как остальные узлы,
 head как бы над всем и все что доступно в конечном счете это head
 поэтому назвать его last было неправильно, мы не знаем last, просто так совпало
 заметим что функция называется list_add, а не list_add_head или tail
 дело в том, что head->next всегда указывает на самый последний добавленный элемент
 а head->prev указывает на самый старый добавленный элемент
 поэтому идя по next мы реализуем стек
 а идя по prev реализуем очередь, обрабатывая в первую очередь тех, кто пришел позже всех
 */
#ifndef CONFIG_DEBUG_LIST
static inline void list_add(struct list_head *new, struct list_head *last)
{
	__list_add(new, last, last->next);
}
#else
extern void list_add(struct list_head *new, struct list_head *last);
#endif


/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
 /*
  я так и думал, что добавление в конец списка это значит вставить элемент между предыдущим и головой
  в сущности понятия tail тоже нет. ведь из head можно пойти по next и тогда это стек
  а можно пойти влево prev и тогда это обработка очереди
  фактически тут добавляется элемент между head->prev (самым старым) и head
  но тут все зависит от того, как написан макрос обхода элементов
  возможно для того, чтобы макрос правильно работал и нужна эта функция
  */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add_rcu(struct list_head * new,
		struct list_head * prev, struct list_head * next)
{
	new->next = next;
	new->prev = prev;
	smp_wmb();
	next->prev = new;
	prev->next = new;
}

/**
 * list_add_rcu - add a new entry to rcu-protected list
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 *
 * The caller must take whatever precautions are necessary
 * (such as holding appropriate locks) to avoid racing
 * with another list-mutation primitive, such as list_add_rcu()
 * or list_del_rcu(), running on this same list.
 * However, it is perfectly legal to run concurrently with
 * the _rcu list-traversal primitives, such as
 * list_for_each_entry_rcu().
 */
static inline void list_add_rcu(struct list_head *new, struct list_head *head)
{
	__list_add_rcu(new, head, head->next);
}

/**
 * list_add_tail_rcu - add a new entry to rcu-protected list
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 *
 * The caller must take whatever precautions are necessary
 * (such as holding appropriate locks) to avoid racing
 * with another list-mutation primitive, such as list_add_tail_rcu()
 * or list_del_rcu(), running on this same list.
 * However, it is perfectly legal to run concurrently with
 * the _rcu list-traversal primitives, such as
 * list_for_each_entry_rcu().
 */
static inline void list_add_tail_rcu(struct list_head *new,
					struct list_head *head)
{
	__list_add_rcu(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!

 просто удаляем элемент
 просто напросто указатели в обе стороны перебиваются
 написано что это внутренняя функция
 */
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	// важная штука оказывается. именно prev сначала удаляется, prev это важный признак в list_empty_careful
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.

 list_empty какая-то
 как круто, не надо обходить весь список и искать элемент
 в сущности он у нас уже есть, можно удалять
 кстати, в процессе итерации можно удалять сразу элементы, надеюсь там next удаляемого элемента запоминается
 хотя и не факт. ведь цикл крутится пока entry->next не упрется в голову списка, плюс в конец pos = pos->next
 к сожалению после выполнения этой функции entry next будут указывать на ядовитые указатели
 надо будет это проверить
 возможно все удаляемые списки надо собирать отдель и потом проходить по списку
 т.е. выделять память
 да, непростая это вещь, удалять элемент из списка
 но по крайней мере удобно что обходить весь в поисках не надо
 но наверное после удаления нужно заново обходить список, потому что entry будет запорото
 эта просто удаляет, не обращая внимание на SMP

 там ниже будет empty функция
 вообще говоря тут написано что empty на этом элементе больше не работает
 кстати, может быть поэтому разные POISON значения
 */
#ifndef CONFIG_DEBUG_LIST
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}
#else
extern void list_del(struct list_head *entry);
#endif

/**
 * list_del_rcu - deletes entry from list without re-initialization (вот оно, сказано что позволяет удалять без прокрутки заново)
 * @entry: the element to delete from the list.

   и да, что будет если список меняют одновременно, ведь нужна блокировка в этом случае
   если уж это RCU список

 *
 * Note: list_empty() on entry does not return true after this,
 * the entry is in an undefined state. It is useful for RCU based
 * lockfree traversal.
 *
 * In particular, it means that we can not poison the forward
 * pointers that may still be used for walking the list.
 *
 * The caller must take whatever precautions are necessary
 * (such as holding appropriate locks) to avoid racing
 * with another list-mutation primitive, such as list_del_rcu()
 * or list_add_rcu(), running on this same list.
 * However, it is perfectly legal to run concurrently with
 * the _rcu list-traversal primitives, such as
 * list_for_each_entry_rcu().

   написано что если использовать _rcu итераторы, то менять список можно одновременно

 *
 * Note that the caller is not permitted to immediately free
 * the newly deleted entry.  Instead, either synchronize_rcu()
 * or call_rcu() must be used to defer freeing until an RCU
 * grace period has elapsed.

   интересная вещь написано, что нельзя просто так сразу выкинуть элемент
   какие-то синхронизаторы должны быть еще
   и еще какой-то период

 */
static inline void list_del_rcu(struct list_head *entry)
{
	// не понимаю, зачем еще присваивать LIST_POISON2?
	__list_del(entry->prev, entry->next);
	entry->prev = LIST_POISON2;
}

/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 ну, тут понятно, просто переприсваиваем соседей
 единственно насчет empty не понял, не понимаю и еще все это меняется без блокировок
 */
static inline void list_replace(struct list_head *old,
				struct list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

/*
выбрасывает элемент и у выброшенного сбрасывает его prev/next
вот собственно образец того, что нужно вызывать в коде и нельзя просто присваивание сделать
*/
static inline void list_replace_init(struct list_head *old,
					struct list_head *new)
{
	list_replace(old, new);
	INIT_LIST_HEAD(old);
}

/**
 * list_replace_rcu - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * The @old entry will be replaced with the @new entry atomically.
 * Note: @old should not be empty.
 */
static inline void list_replace_rcu(struct list_head *old,
				struct list_head *new)
{
	new->next = old->next;
	new->prev = old->prev;
	smp_wmb();
	new->next->prev = new;
	new->prev->next = new;
	old->prev = LIST_POISON2; // зачем-то опять это здесь
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 ну удаляется элемент, вся разница в том, что entry очищается от всех ссылок, где-то используется наверное
 хотя казалось бы выкинут, ну и ладно
 только об этом подумал, о перемещений в другой список
 */
static inline void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 странно, что написан list, вроде как должно быть entry
 но функция переносит элемент из одного списка в другой, именно удаляя его
 добавляет в начало списка
 */
static inline void list_move(struct list_head *list, struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 тоже самое что и предыдущая функция, только на этот раз добавление идет в хвост
 */
static inline void list_move_tail(struct list_head *list,
				  struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add_tail(list, head);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 здесь константы описаны, возможно это связано с inline и помощь компилятору в оптимизации
 мы как бы подсказываем что аргументы не меняются, компилятор это проверяет
 и далее можно делать inline оптимизации
 а так проверяется, что элемент последний
 последний элемент это условность, но в любом случае указывает на голову списка
 */
static inline int list_is_last(const struct list_head *list,
				const struct list_head *head)
{
	return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 проверяет, что список пустой
 ну, список пустой пока голова указывает сама на себя
 тут еще дело в том, что пустой список или нет можно проверить на любом элементе
 у любого элемента next не является он сам
 все равно непонятно зачем два POISON, ведь head-то не меняется
 */
static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

/**
 * list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 да, тут описано не был ли он модифицирован, а не модифицируется прямо сейчас
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.

   что-то очень важно описано и видимо тут используется какая-то магия в последовательности изменения списка несколькими процессорами
   я тут вижу что написано что делается проверка что другой процессор ничего не меняет, но вроде как все это построено на алгоритме доступа
   магия!
   да, второе условие для этого и создано
   сначала считывается следующий элемент
   далее проверяется что он не совпадает с головой и тут вроде как все нормально
   но! есть второе условие
   список пустой если следующий элемент это мы сами, но может оказаться так что после этого другой процессор все обновил!
   почему сравнивается next во втором условии, а не head
   суть в том, что пока мы считали первое условие, наш head поменяли, поменяли поля next и prev
   и вторая проверка просто удостоверивается в том, что список не поменяли
   ну, это не отменяет проблему ABA
   я так понимаю что prev должен меняться в самую последнюю очередь?
   или наоборот, в самую первую как указание на то, что началось изменение
   когда вызывается list_del_init, то удаляемый элемент замыкается сам на себя
   херня какая-то
 */
static inline int list_empty_careful(const struct list_head *head)
{
	struct list_head *next = head->next;
	return (next == head) && (next == head->prev);
}

// добавляет список в начало другого списка
static inline void __list_splice(struct list_head *list,
				 struct list_head *head)
{
	// голова не добавляется
	// голова вообще может быть статической
	// кстати занятная штука, старый список не очищается
	// а интересно, можно добавить часть списка, вырезать часть списка и добавить?
	// наверное нельзя потому что будет добавлено все что после элемента, а не начиная с ...
	struct list_head *first = list->next;
	struct list_head *last = list->prev;
	struct list_head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
}

/**
 * list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice(struct list_head *list, struct list_head *head)
{
	if (!list_empty(list)) // непонятно, но это видимо чтобы не добавлять голову, все равно список пустой
		__list_splice(list, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 добавляет в список, при этом старый список зануляется
 зачем вообще это надо, может быть для перекачки списков из одной очереди в другую
 */
static inline void list_splice_init(struct list_head *list,
				    struct list_head *head)
{
	if (!list_empty(list)) {
		__list_splice(list, head);
		INIT_LIST_HEAD(list);
	}
}

/**
 * list_splice_init_rcu - splice an RCU-protected list into an existing list.
 * @list:	the RCU-protected list to splice
 * @head:	the place in the list to splice the first list into
 * @sync:	function to sync: synchronize_rcu(), synchronize_sched(), ...
 *
 * @head can be RCU-read traversed concurrently with this function.
 *
 * Note that this function blocks.
 *
 * Important note: the caller must take whatever action is necessary to
 *	prevent any other updates to @head.  In principle, it is possible
 *	to modify the list as soon as sync() begins execution.
 *	If this sort of thing becomes necessary, an alternative version
 *	based on call_rcu() could be created.  But only if -really-
 *	needed -- there is no shortage of RCU API members.
 */
static inline void list_splice_init_rcu(struct list_head *list,
					struct list_head *head,
					void (*sync)(void))
{
	struct list_head *first = list->next;
	struct list_head *last = list->prev;
	struct list_head *at = head->next;

	if (list_empty(head))
		return;

	/* "first" and "last" tracking list, so initialize it. */

	INIT_LIST_HEAD(list);

	/*
	 * At this point, the list body still points to the source list.
	 * Wait for any readers to finish using the list before splicing
	 * the list body into the new list.  Any new readers will see
	 * an empty list.
	 */

	sync();

	/*
	 * Readers are finished with the source list, so perform splice.
	 * The order is important if the new list is global and accessible
	 * to concurrent RCU readers.  Note that RCU readers are not
	 * permitted to traverse the prev pointers without excluding
	 * this function.
	 */

	last->next = at;
	smp_wmb();
	head->next = first;
	first->prev = head;
	at->prev = last;
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 крутая штука, возвращает ссылку на объект-контейнер зная только list_head entry
 */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 возвращает первый элемент из списка, последний добавленный
 */
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
/*
 основная функция по обходу списка
 передется имя переменной типа указатель куда будет сбрасываться указатель на определенный элемент итерации
 и передается список, точнее указатель на него. везде и всегда передается только указатель на список, а не он сам
 т.е. для переменных нужно использовать &
 да, основной итератор, в pos будет получен list_head элемент
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; prefetch(pos->next), pos != (head); \
        	pos = pos->next)

/**
 * __list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 *
 * This variant differs from list_for_each() in that it's the
 * simplest possible list iteration code, no prefetching is done.
 * Use this for code that knows the list to be very short (empty
 * or 1 entry) most of the time.
 вариант без prefetch, написано использовать когда список очень короткий и чаще из одного элемента
 смысла использовать prefetch в этом случае нет
 */
#define __list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 итерация в обратном порядке, фактически в порядке добавлния элементов в список
 */
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
        	pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 то, о чем я думал. итерация в тот момент когда элемент удален
 используя этот макрос можно удалять элементы из списка
 макрос реализован за счет использования дополнительной n переменной куда сохраняется следующий элемент
 мы просто восстанавливаем потом pos из n
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 такая же фигня только в обратную сторону, теперь в n сохраняется prev
 только тут работает prefetch, в прежнем методе я этого не видел
 */
#define list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     prefetch(pos->prev), pos != (head); \
	     pos = n, n = pos->prev)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 тоже крутая штука, только в этом случае итерация идет сразу по контейнеру
 т.е. в pos можно получить сразу указатель на контейнер
 */
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     prefetch(pos->member.next), &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 тоже самое в обратном порядке
 */
#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_entry((head)->prev, typeof(*pos), member);	\
	     prefetch(pos->member.prev), &pos->member != (head); 	\
	     pos = list_entry(pos->member.prev, typeof(*pos), member))

/**
 * list_prepare_entry - prepare a pos entry for use in list_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the list_struct within the struct.
 *
 * Prepares a pos entry for use as a start point in list_for_each_entry_continue().
 херня какая-то, используется ?:
 готовит первый элемент из головы списка
 а прикольно, в качестве true значения возвращается сам же pos при этом это не указано!
 да, нужно готовить с какого элемента начнется итерация, либо с pos, либо с головы
 */
#define list_prepare_entry(pos, head, member) \
	((pos) ? : list_entry(head, typeof(*pos), member))

/**
 * list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 я понял, есть элемент и он часть списка
 позволяет итерацию начиная с этого элемента, а не с головы списка
 видимо предыдущий метод готовит элемент для вращения
 т.е. если есть, то используется как есть, иначе создается из головы
 итерация начиная со следующего элемента
 возможно в новых версиях list.h уже поменялся
 */
#define list_for_each_entry_continue(pos, head, member) 		\
	for (pos = list_entry(pos->member.next, typeof(*pos), member);	\
	     prefetch(pos->member.next), &pos->member != (head);	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 тоже самое только в обратном порядке
 я понял разницу. тут идет итерация со следующего элемента
 */
#define list_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = list_entry(pos->member.prev, typeof(*pos), member);	\
	     prefetch(pos->member.prev), &pos->member != (head);	\
	     pos = list_entry(pos->member.prev, typeof(*pos), member))

/**
 * list_for_each_entry_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 pos настраивается на текущую позицию и будет первым же элементом в итерации
 */
#define list_for_each_entry_from(pos, head, member) 			\
	for (; prefetch(pos->member.next), &pos->member != (head);	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 итерация по каждому контейнеру начиная с головы с учетом возможных удалении
 */
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_continue
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 безопасная итерация после текущего элемента с учетом возможных удалении
 */
#define list_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = list_entry(pos->member.next, typeof(*pos), member), 		\
		n = list_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_from
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 итерация начиная с некоторой точки включая ее же саму с учетом возможных удалении
 */
#define list_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = list_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_reverse
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 тоже самое, только в обратном порядке начиная с некоторой точки включая ее
 */
#define list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = list_entry((head)->prev, typeof(*pos), member),	\
		n = list_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.prev, typeof(*n), member))

/**
 * list_for_each_rcu	-	iterate over an rcu-protected list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 *
 * This list-traversal primitive may safely run concurrently with
 * the _rcu list-mutation primitives such as list_add_rcu()
 * as long as the traversal is guarded by rcu_read_lock().
 */
#define list_for_each_rcu(pos, head) \
	for (pos = (head)->next; \
		prefetch(rcu_dereference(pos)->next), pos != (head); \
        	pos = pos->next)

#define __list_for_each_rcu(pos, head) \
	for (pos = (head)->next; \
		rcu_dereference(pos) != (head); \
        	pos = pos->next)

/**
 * list_for_each_safe_rcu
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 *
 * Iterate over an rcu-protected list, safe against removal of list entry.
 *
 * This list-traversal primitive may safely run concurrently with
 * the _rcu list-mutation primitives such as list_add_rcu()
 * as long as the traversal is guarded by rcu_read_lock().
 */
#define list_for_each_safe_rcu(pos, n, head) \
	for (pos = (head)->next; \
		n = rcu_dereference(pos)->next, pos != (head); \
		pos = n)

/**
 * list_for_each_entry_rcu	-	iterate over rcu list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * This list-traversal primitive may safely run concurrently with
 * the _rcu list-mutation primitives such as list_add_rcu()
 * as long as the traversal is guarded by rcu_read_lock().
 */
#define list_for_each_entry_rcu(pos, head, member) \
	for (pos = list_entry((head)->next, typeof(*pos), member); \
		prefetch(rcu_dereference(pos)->member.next), \
			&pos->member != (head); \
		pos = list_entry(pos->member.next, typeof(*pos), member))


/**
 * list_for_each_continue_rcu
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 *
 * Iterate over an rcu-protected list, continuing after current point.
 *
 * This list-traversal primitive may safely run concurrently with
 * the _rcu list-mutation primitives such as list_add_rcu()
 * as long as the traversal is guarded by rcu_read_lock().
 */
#define list_for_each_continue_rcu(pos, head) \
	for ((pos) = (pos)->next; \
		prefetch(rcu_dereference((pos))->next), (pos) != (head); \
        	(pos) = (pos)->next)

/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 какая-то муть. двунаправленный список при этом голова списка имеет только один указатель
 жесть, hlist
 остальные функции пока не смотрел
 */

struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};

#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) struct hlist_head name = {  .first = NULL }
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
static inline void INIT_HLIST_NODE(struct hlist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static inline int hlist_unhashed(const struct hlist_node *h)
{
	return !h->pprev;
}

static inline int hlist_empty(const struct hlist_head *h)
{
	return !h->first;
}

static inline void __hlist_del(struct hlist_node *n)
{
	struct hlist_node *next = n->next;
	struct hlist_node **pprev = n->pprev;
	*pprev = next;
	if (next)
		next->pprev = pprev;
}

static inline void hlist_del(struct hlist_node *n)
{
	__hlist_del(n);
	n->next = LIST_POISON1;
	n->pprev = LIST_POISON2;
}

/**
 * hlist_del_rcu - deletes entry from hash list without re-initialization
 * @n: the element to delete from the hash list.
 *
 * Note: list_unhashed() on entry does not return true after this,
 * the entry is in an undefined state. It is useful for RCU based
 * lockfree traversal.
 *
 * In particular, it means that we can not poison the forward
 * pointers that may still be used for walking the hash list.
 *
 * The caller must take whatever precautions are necessary
 * (such as holding appropriate locks) to avoid racing
 * with another list-mutation primitive, such as hlist_add_head_rcu()
 * or hlist_del_rcu(), running on this same list.
 * However, it is perfectly legal to run concurrently with
 * the _rcu list-traversal primitives, such as
 * hlist_for_each_entry().
 */
static inline void hlist_del_rcu(struct hlist_node *n)
{
	__hlist_del(n);
	n->pprev = LIST_POISON2;
}

static inline void hlist_del_init(struct hlist_node *n)
{
	if (!hlist_unhashed(n)) {
		__hlist_del(n);
		INIT_HLIST_NODE(n);
	}
}

/**
 * hlist_replace_rcu - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * The @old entry will be replaced with the @new entry atomically.
 */
static inline void hlist_replace_rcu(struct hlist_node *old,
					struct hlist_node *new)
{
	struct hlist_node *next = old->next;

	new->next = next;
	new->pprev = old->pprev;
	smp_wmb();
	if (next)
		new->next->pprev = &new->next;
	*new->pprev = new;
	old->pprev = LIST_POISON2;
}

static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
	struct hlist_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}


/**
 * hlist_add_head_rcu
 * @n: the element to add to the hash list.
 * @h: the list to add to.
 *
 * Description:
 * Adds the specified element to the specified hlist,
 * while permitting racing traversals.
 *
 * The caller must take whatever precautions are necessary
 * (such as holding appropriate locks) to avoid racing
 * with another list-mutation primitive, such as hlist_add_head_rcu()
 * or hlist_del_rcu(), running on this same list.
 * However, it is perfectly legal to run concurrently with
 * the _rcu list-traversal primitives, such as
 * hlist_for_each_entry_rcu(), used to prevent memory-consistency
 * problems on Alpha CPUs.  Regardless of the type of CPU, the
 * list-traversal primitive must be guarded by rcu_read_lock().
 */
static inline void hlist_add_head_rcu(struct hlist_node *n,
					struct hlist_head *h)
{
	struct hlist_node *first = h->first;
	n->next = first;
	n->pprev = &h->first;
	smp_wmb();
	if (first)
		first->pprev = &n->next;
	h->first = n;
}

/* next must be != NULL */
static inline void hlist_add_before(struct hlist_node *n,
					struct hlist_node *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	*(n->pprev) = n;
}

static inline void hlist_add_after(struct hlist_node *n,
					struct hlist_node *next)
{
	next->next = n->next;
	n->next = next;
	next->pprev = &n->next;

	if(next->next)
		next->next->pprev  = &next->next;
}

/**
 * hlist_add_before_rcu
 * @n: the new element to add to the hash list.
 * @next: the existing element to add the new element before.
 *
 * Description:
 * Adds the specified element to the specified hlist
 * before the specified node while permitting racing traversals.
 *
 * The caller must take whatever precautions are necessary
 * (such as holding appropriate locks) to avoid racing
 * with another list-mutation primitive, such as hlist_add_head_rcu()
 * or hlist_del_rcu(), running on this same list.
 * However, it is perfectly legal to run concurrently with
 * the _rcu list-traversal primitives, such as
 * hlist_for_each_entry_rcu(), used to prevent memory-consistency
 * problems on Alpha CPUs.
 */
static inline void hlist_add_before_rcu(struct hlist_node *n,
					struct hlist_node *next)
{
	n->pprev = next->pprev;
	n->next = next;
	smp_wmb();
	next->pprev = &n->next;
	*(n->pprev) = n;
}

/**
 * hlist_add_after_rcu
 * @prev: the existing element to add the new element after.
 * @n: the new element to add to the hash list.
 *
 * Description:
 * Adds the specified element to the specified hlist
 * after the specified node while permitting racing traversals.
 *
 * The caller must take whatever precautions are necessary
 * (such as holding appropriate locks) to avoid racing
 * with another list-mutation primitive, such as hlist_add_head_rcu()
 * or hlist_del_rcu(), running on this same list.
 * However, it is perfectly legal to run concurrently with
 * the _rcu list-traversal primitives, such as
 * hlist_for_each_entry_rcu(), used to prevent memory-consistency
 * problems on Alpha CPUs.
 */
static inline void hlist_add_after_rcu(struct hlist_node *prev,
				       struct hlist_node *n)
{
	n->next = prev->next;
	n->pprev = &prev->next;
	smp_wmb();
	prev->next = n;
	if (n->next)
		n->next->pprev = &n->next;
}

#define hlist_entry(ptr, type, member) container_of(ptr,type,member)

#define hlist_for_each(pos, head) \
	for (pos = (head)->first; pos && ({ prefetch(pos->next); 1; }); \
	     pos = pos->next)

#define hlist_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
	     pos = n)

/**
 * hlist_for_each_entry	- iterate over list of given type
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hlist_node to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry(tpos, pos, head, member)			 \
	for (pos = (head)->first;					 \
	     pos && ({ prefetch(pos->next); 1;}) &&			 \
		({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = pos->next)

/**
 * hlist_for_each_entry_continue - iterate over a hlist continuing after current point
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hlist_node to use as a loop cursor.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_continue(tpos, pos, member)		 \
	for (pos = (pos)->next;						 \
	     pos && ({ prefetch(pos->next); 1;}) &&			 \
		({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = pos->next)

/**
 * hlist_for_each_entry_from - iterate over a hlist continuing from current point
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hlist_node to use as a loop cursor.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_from(tpos, pos, member)			 \
	for (; pos && ({ prefetch(pos->next); 1;}) &&			 \
		({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = pos->next)

/**
 * hlist_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hlist_node to use as a loop cursor.
 * @n:		another &struct hlist_node to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_safe(tpos, pos, n, head, member) 		 \
	for (pos = (head)->first;					 \
	     pos && ({ n = pos->next; 1; }) && 				 \
		({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = n)

/**
 * hlist_for_each_entry_rcu - iterate over rcu list of given type
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hlist_node to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 *
 * This list-traversal primitive may safely run concurrently with
 * the _rcu list-mutation primitives such as hlist_add_head_rcu()
 * as long as the traversal is guarded by rcu_read_lock().
 */
#define hlist_for_each_entry_rcu(tpos, pos, head, member)		 \
	for (pos = (head)->first;					 \
	     rcu_dereference(pos) && ({ prefetch(pos->next); 1;}) &&	 \
		({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = pos->next)

#else
#warning "don't include kernel headers in userspace"
#endif /* __KERNEL__ */
#endif
