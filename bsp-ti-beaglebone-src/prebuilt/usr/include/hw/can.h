#ifndef __CAN_H_INCLUDED
#define __CAN_H_INCLUDED

#include <stdint.h>
#include <inttypes.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/can_dcmd.h>

/* Event Flag Definitions */
#define CANDEV_EVENT_QUEUED 	0x00000001
/* #define CANDEV_EVENT_ 		0x00000020 */
/* #define CANDEV_EVENT_ 		0x00000040 */
/* #define CANDEV_EVENT_ 		0x00000080 */
/* #define CANDEV_EVENT_ 		0x00000100 */

/* CAN Message Linked List Data Structure */
typedef struct _canmsg {
	CAN_MSG 			cmsg; 		/* CAN message */
	struct _canmsg 		*next; 		/* Pointer to next message */
	struct _canmsg 		*prev; 		/* Pointer to previous message */
} canmsg_t;

/* Message List Data Structure */
typedef struct _canmsg_list {
	size_t 				len; 		/* Length of each msg in bytes */
	unsigned 			nmsg; 		/* Total number of messages in list */
	volatile unsigned 	msgcnt; 	/* Number of messages in use */
	canmsg_t 			*msgs; 		/* Array of messages defining the message list */
	canmsg_t 			*head; 		/* Head of list to add new messages */
	canmsg_t 			*tail; 		/* Tail of list to consume messages */
	intrspin_t 			lock; 		/* List locking mechanism */
} canmsg_list_t;

/* CAN Device Type - either transmit or receive */
typedef enum
{
	CANDEV_TYPE_RX,
	CANDEV_TYPE_TX
} CANDEV_TYPE;

/* Blocked Client Wait Structure */
typedef struct client_wait_entry
{
	int 						rcvid;	/* Blocked client's rcvid */
	struct client_wait_entry 	*next;	/* Pointer to next blocked client */
} CLIENTWAIT;

/* Blocked Client Wait Queue */
typedef struct client_waitq_entry
{
	CLIENTWAIT					*wait;	/* Head of client wait queue */
	int							cnt;	/* Number of clients waiting */
} CLIENTWAITQ;

/* CAN Init Device Structure */
typedef struct can_dev_init_entry
{
	CANDEV_TYPE 		devtype;		/* CAN device type */
	int 				can_unit; 		/* CAN unit number */
	int 				dev_unit; 		/* Device unit number */
	uint32_t 			msgnum; 		/* Number of messages */
	uint32_t 			datalen; 		/* Length of CAN message data */
} CANDEV_INIT;

/* Generic CAN Device Structure */
typedef struct can_dev_entry
{
	iofunc_attr_t 		attr;
	iofunc_mount_t 		mount;
	CANDEV_TYPE 		devtype;		/* CAN device type */
	int 				can_unit; 		/* CAN unit number */
	int 				dev_unit; 		/* Device unit number */
	canmsg_list_t		*msg_list;		/* Device message list */
	volatile uint32_t 	cflags; 		/* CAN device flags */
	struct sigevent 	event;			/* Device event */
	CLIENTWAITQ			waitq;			/* Client wait queue */
} CANDEV;

/* CAN Device Driver Implemented Functions */
typedef struct _can_drvr_funcs_t
{
	void 	(*transmit)(CANDEV *cdev);
	int 	(*devctl)(CANDEV *cdev, int dcmd, DCMD_DATA *data);
} can_drvr_funcs_t;

/* Resource Manager Info Structure */
typedef struct resmgr_info_entry
{
	dispatch_t 					*dpp;
	dispatch_context_t 			*ctp;
	int							coid;
	resmgr_attr_t 				resmgr_attr;
	resmgr_connect_funcs_t 		connect_funcs;
	resmgr_io_funcs_t 			io_funcs;
	can_drvr_funcs_t			*drvr_funcs;
} RESMGR_INFO;

/* Global resource manager info variable */
extern RESMGR_INFO rinfo;

/* Library functions available to driver */
void can_resmgr_init(can_drvr_funcs_t *drvr_funcs);
void can_resmgr_start(void);
void can_resmgr_create_device(CANDEV *cdev);
void can_resmgr_init_device(CANDEV *cdev, CANDEV_INIT *cinit);
struct sigevent *can_client_check(CANDEV *cdev);

/* Message list function prototypes */
canmsg_list_t *msg_list_create(unsigned nmsg, size_t len);
void msg_list_destroy(canmsg_list_t *ml);

/* Enqueue/Dequeue message from/to clients for use with io_write/io_read */
int msg_enqueue_client(canmsg_list_t *ml, resmgr_context_t *ctp, int offset);
int msg_dequeue_client(canmsg_list_t *ml, resmgr_context_t *ctp, int offset);

/* Enqueue/Dequeue the next message */
void msg_enqueue(canmsg_list_t *ml);
void msg_dequeue(canmsg_list_t *ml);

/* Get the next Enqueue/Dequeue message */
canmsg_t *msg_enqueue_next(canmsg_list_t *ml);
canmsg_t *msg_dequeue_next(canmsg_list_t *ml);

/* Mini-driver support functions */
canmsg_t *msg_enqueue_mdriver_next(canmsg_list_t *ml);
void msg_enqueue_mdriver(canmsg_list_t *ml);


#endif


__SRCVERSION( "$URL: http://svn/product/tags/internal/bsp/nto650/ti-am3517-evm/1.0.0/latest/lib/io-can/public/hw/can.h $ $Rev: 219996 $" )
