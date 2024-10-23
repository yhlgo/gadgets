#ifndef YU_LIST_D_H
#define YU_LIST_D_H

/*
 * A ring implementation based on an embedded circular doubly-linked list.
 *
 * typedef struct my {
 *   int data;
 *   qr(struct my) _dl;
 * }my_s;
 */

//Ring definitions.
#define dlist(_type)          \
	struct {              \
		_type *_next; \
		_type *_prev; \
	}

//Initialize a qr link.
#define dl_init(dl, _dl)                \
	do {                            \
		(dl)->_dl._next = (dl); \
		(dl)->_dl._prev = (dl); \
	} while (0)

//Go forwards or backwards in the ring.
#define dl_next(dl, _dl) ((dl)->_dl._next)
#define dl_prev(dl, _dl) ((dl)->_dl._prev)

/*
 * Given two rings:
 *    a -> a_1 -> ... -> a_n -> a
 *    ^                         |
 *
 *    b -> b_1 -> ... -> b_n -> b
 *    ^                         |
 *
 * Results in the ring:
 *   a -> a_1 -> ... -> a_n -> b -> b_1 -> ... -> b_n -> a
 *   ^                                                   |
 *
 * dl_a can directly be a dl_next() macro, but dl_b cannot.
 */
#define dl_meld(dl_a, dl_b, _dl)                                  \
	do {                                                      \
		(dl_b)->_dl._prev->_dl._next = (dl_a)->_dl._prev; \
		(dl_a)->_dl._prev = (dl_b)->_dl._prev;            \
		(dl_b)->_dl._prev = (dl_b)->_dl._prev->_dl._next; \
		(dl_a)->_dl._prev->_dl._next = (dl_a);            \
		(dl_b)->_dl._prev->_dl._next = (dl_b);            \
	} while (0)

//Logically, this is just a meld. dl_elem is a single-element ring.
#define dl_before_insert(dl, dl_elem, _dl) dl_meld((dl), (dl_elem), _dl)

//Ditto, but inserting after rather than before.
#define dl_after_insert(dl, dl_elem, _dl) dl_before_insert(dl_next(dl, _dl), (dl_elem), _dl)

#define dl_split(dl_a, dl_b, _dl) dl_meld((dl_a), (dl_b), _dl)

//Splits off dl from the rest of its ring, so that it becomes a single-element ring.
#define dl_remove(dl, _dl) dl_split(dl_next(dl, _dl), (dl), _dl)

/*
 * Helper macro to iterate over each element in a ring exactly once, starting
 * with dl.  The usage is (assuming my_t defined as above):
 *
 * int sum(my_t *dl) {
 *   int sum = 0;
 *   my_t *iter;
 *   dl_foreach(iter, dl, _dl) {
 *     sum += iter->data;
 *   }
 *   return sum;
 * }
 */
#define dl_foreach(var, dl, _dl) for ((var) = (dl); (var) != NULL; (var) = (((var)->_dl._next != (dl)) ? (var)->_dl._next : NULL))

//The same (and with the same usage) as dl_foreach, but in the opposite order, ending with dl.
#define dl_reverse_foreach(var, dl, _dl) \
	for ((var) = ((dl) != NULL) ? dl_prev(dl, _dl) : NULL; (var) != NULL; (var) = (((var) != (dl)) ? (var)->_dl._prev : NULL))

#endif
