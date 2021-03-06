#ifndef O_NATIVE_SOCKET_SELECTOR_H_
#define O_NATIVE_SOCKET_SELECTOR_H_
#include "o_native_socket.h"


struct o_native_socket_selector;

/*! \brief Create a new socket selector.
 *
 * \param mode the mode of select
 * \return the new selector.
 */
struct o_native_socket_selector * o_native_socket_selector_new();

/*! \brief add a socket to the selector.
 *
 * \param selector where add a socket.
 * \param sock to add.
 */
void o_native_socket_selector_add_socket(struct o_native_socket_selector * selector, struct o_native_socket* sock);

/*! \brief Remove a socket from the selector.
 *
 * \param selector where remove.
 * \param sock to remove.
 */
void o_native_socket_selector_remove_socket(struct o_native_socket_selector * selector, struct o_native_socket* sock);

/*! \brief Start the select of socket.
 *
 * \param selector to select.
 * \param select timeout.
 * \return an selected socket or 0 if select fail.
 */
struct o_native_socket* o_native_socket_selector_select(struct o_native_socket_selector * selector, int timeout);

/*! \brief Readd the socket in the selector when the operation on socket was ended.
 *
 * \param selector where add
 * \param sock to add
 */
void o_native_socket_selector_end_select(struct o_native_socket_selector * selector, struct o_native_socket* sock);

/*!\brief free the selector.
 *
 * \param selector to free.
 */
void o_native_socket_selector_free(struct o_native_socket_selector * selector);

#endif /* O_NATIVE_SOCKET_SELECTOR_H_ */
