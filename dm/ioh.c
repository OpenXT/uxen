
#include "config.h"

#include <stdint.h>

#include "queue.h"
#include "ioh.h"

void ioh_queue_init(struct io_handler_queue *iohq)
{
    TAILQ_INIT(&iohq->queue);
    critical_section_init(&iohq->lock);
    iohq->wait_queue = NULL;
}

#ifdef CONFIG_NETEVENT

/* XXX: fd_read_poll should be suppressed, but an API change is
   necessary in the character devices to suppress fd_can_read(). */

int ioh_set_read_handler2(int fd,
                          struct io_handler_queue *iohq,
                          IOCanRWHandler *fd_read_poll,
                          IOHandler *fd_read,
                          void *opaque)
{
    IOHandlerRecord *ioh;

    if (!iohq)
        iohq = &io_handlers;

    critical_section_enter(&iohq->lock);
    TAILQ_FOREACH(ioh, &iohq->queue, queue)
	if (ioh->fd == fd && ioh->deleted == 0)
	    break;

    if (!fd_read && ioh && !ioh->fd_write)
        ioh->deleted = 1;
    else {
	if (!ioh) {
	    ioh = calloc(1, sizeof(IOHandlerRecord));
            ioh->deleted = 0;
            ioh->fd = fd;
            ioh->fd_write_poll = NULL;
            ioh->fd_write = NULL;
            ioh->write_opaque = NULL;
            TAILQ_INSERT_HEAD(&iohq->queue, ioh, queue);
	}
        ioh->read_opaque = opaque;
        ioh->fd_read_poll = fd_read_poll;
        ioh->fd_read = fd_read;
    }
    if (iohq->wait_queue) /* asleep in another thread */
        ioh_wait_interrupt(iohq->wait_queue);
    critical_section_leave(&iohq->lock);
    return 0;
}

int ioh_set_write_handler2(int fd,
                           struct io_handler_queue *iohq,
                           IOCanRWHandler *fd_write_poll,
                           IOHandler *fd_write,
                           void *opaque)
{
    IOHandlerRecord *ioh;

    if (!iohq)
        iohq = &io_handlers;

    critical_section_enter(&iohq->lock);
    TAILQ_FOREACH(ioh, &iohq->queue, queue)
	if (ioh->fd == fd && ioh->deleted == 0)
	    break;

    if (!fd_write && ioh && !ioh->fd_read)
        ioh->deleted = 1;
    else {
	if (!ioh) {
	    ioh = calloc(1, sizeof(IOHandlerRecord));
            ioh->deleted = 0;
            ioh->fd = fd;
            ioh->fd_read_poll = NULL;
            ioh->fd_read = NULL;
            ioh->read_opaque = NULL;
            TAILQ_INSERT_HEAD(&iohq->queue, ioh, queue);
	}
        ioh->write_opaque = opaque;
        ioh->fd_write_poll = fd_write_poll;
        ioh->fd_write = fd_write;
    }
    if (iohq->wait_queue) /* asleep in another thread */
        ioh_wait_interrupt(iohq->wait_queue);
    critical_section_leave(&iohq->lock);
    return 0;
}

#endif  /* CONFIG_NETEVENT */

void ioh_init(void)
{
    ioh_queue_init(&io_handlers);
    ioh_init_wait_objects(&wait_objects);
}
