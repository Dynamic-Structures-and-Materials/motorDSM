/* File: drvMCB4B.cc                    */

/* Device Driver Support routines for motor */
/*
 *      Original Author: Mark Rivers
 *      Date: 2/24/2002
 *
 * Modification Log:
 * -----------------
 * .01  02-24-2002   mlr  initialized from drvPM304.c
 * .02  07-03-2002   rls  replaced RA_OVERTRAVEL with RA_PLUS_LS and RA_MINUS_LS
 * .03  05-23-2003   rls  Converted to R3.14.x.
 */


#include <string.h>
#include <epicsThread.h>
#include <drvSup.h>
#include "motor.h"
#include "AcsRegister.h"
#include "drvMCB4B.h"
#include "serialIO.h"
#include "epicsExport.h"

#define STATIC static

#define WAIT 1

#define SERIAL_TIMEOUT 2000 /* Command timeout in msec */

#define BUFF_SIZE 100       /* Maximum length of string to/from MCB4B */

#ifdef __GNUG__
    #ifdef DEBUG
	volatile int drvMCB4BDebug = 0;
	#define Debug(L, FMT, V...) { if(L <= drvMCB4BDebug) \
                        { errlogPrintf("%s(%d):",__FILE__,__LINE__); \
                          errlogPrintf(FMT,##V); } }
    #else
	#define Debug(L, FMT, V...)
    #endif
#else
    #define Debug()
#endif

/* Debugging notes:
 *   drvMCB4BDebug == 0  No debugging information is printed
 *   drvMCB4BDebug >= 1  Warning information is printed
 *   drvMCB4BDebug >= 2  Time-stamped messages are printed for each string 
 *                       sent to and received from the controller
 *   drvMCB4BDebug >= 3  Additional debugging messages
 */    

int MCB4B_num_cards = 0;

/* Local data required for every driver; see "motordrvComCode.h" */
#include        "motordrvComCode.h"


/*----------------functions-----------------*/
STATIC int recv_mess(int, char *, int);
STATIC RTN_STATUS send_mess(int card, const char *com, char c);
STATIC void start_status(int card);
STATIC int set_status(int card, int signal);
static long report(int level);
static long init();
STATIC int motor_init();
STATIC void query_done(int, int, struct mess_node *);

/*----------------functions-----------------*/

struct driver_table MCB4B_access =
{
    motor_init,
    motor_send,
    motor_free,
    motor_card_info,
    motor_axis_info,
    &mess_queue,
    &queue_lock,
    &free_list,
    &freelist_lock,
    &motor_sem,
    &motor_state,
    &total_cards,
    &any_motor_in_motion,
    send_mess,
    recv_mess,
    set_status,
    query_done,
    start_status,
    &initialized
};

struct
{
    long number;
#ifdef __cplusplus
    long (*report) (int);
    long (*init) (void);
#else
    DRVSUPFUN report;
    DRVSUPFUN init;
#endif
} drvMCB4B = {2, report, init};

epicsExportAddress(drvet, drvMCB4B);

STATIC struct thread_args targs = {SCAN_RATE, &MCB4B_access};


/*********************************************************
 * Print out driver status report
 *********************************************************/
static long report(int level)
{
  int card;

  if (MCB4B_num_cards <=0)
    printf("    NO MCB4B controllers found\n");
  else
    {
      for (card = 0; card < MCB4B_num_cards; card++)
          if (motor_state[card])
             printf("    MCB4B controller %d, id: %s \n",
                   card,
                   motor_state[card]->ident);
    }
  return (0);
}


static long init()
{
   /*
    * We cannot call motor_init() here, because that function can do GPIB I/O,
    * and hence requires that the drvGPIB have already been initialized.
    * That cannot be guaranteed, so we need to call motor_init from device
    * support
    */
    /* Check for setup */
    if (MCB4B_num_cards <= 0)
    {
        Debug(1, "init: *MCB4B driver disabled*\n");
        Debug(1, "MCB4BSetup() is missing from startup script.\n");
        return (ERROR);
    }

    return ((long) 0);
}

STATIC void query_done(int card, int axis, struct mess_node *nodeptr)
{
}


/*********************************************************
 * Read the status and position of all motors on a card
 * start_status(int card)
 *            if card == -1 then start all cards
 *********************************************************/
STATIC void start_status(int card)
{
    /* The MCB4B cannot query status or positions of all axes with a
     * single command.  This needs to be done on an axis-by-axis basis,
     * so this function does nothing
     */
}


/**************************************************************
 * Query position and status for an axis
 * set_status()
 ************************************************************/

STATIC int set_status(int card, int signal)
{
    register struct mess_info *motor_info;
    char command[BUFF_SIZE];
    char response[BUFF_SIZE];
    struct mess_node *nodeptr;
    int rtn_state;
    long motorData;
    char buff[BUFF_SIZE];
    bool ls_active = false;

    motor_info = &(motor_state[card]->motor_info[signal]);
    nodeptr = motor_info->motor_motion;

    /* Request the moving status of this motor */
    sprintf(command, "#%02dX", signal);
    send_mess(card, command, 0);
    recv_mess(card, response, WAIT);
    /* The response string is of the form "#01X=1" */

    if (response[5] == '1')
        motor_info->status &= ~RA_DONE;
    else {
        motor_info->status |= RA_DONE;
    }

    /* Request the limit status of this motor */
    sprintf(command, "#%02dE", signal);
    send_mess(card, command, 0);
    recv_mess(card, response, WAIT);
    /* The response string is of the form "#01E=1" */
    motor_info->status &= ~(RA_PLUS_LS | RA_MINUS_LS);
    if (response[5] == '1') {
        motor_info->status |= RA_PLUS_LS;
        motor_info->status |= RA_DIRECTION;
	ls_active = true;
    }
    if (response[6] == '1') {
        motor_info->status |= RA_MINUS_LS;
        motor_info->status &= ~RA_DIRECTION;
	ls_active = true;
    }

    /* encoder status */
    motor_info->status &= ~EA_SLIP;
    motor_info->status &= ~EA_POSITION;
    motor_info->status &= ~EA_SLIP_STALL;
    motor_info->status &= ~EA_HOME;

    /* Request the position of this motor */
    sprintf(command, "#%02dP", signal);
    send_mess(card, command, 0);
    recv_mess(card, response, WAIT);
    /* The response string is of the form "#01P=+1000" */
    motorData = atoi(&response[5]);

    if (motorData == motor_info->position)
        motor_info->no_motion_count++;
    else
        {
            motor_info->position = motorData;
            motor_info->encoder_position = motorData;
            motor_info->no_motion_count = 0;
        }

    /* Parse motor velocity? */
    /* NEEDS WORK */

    motor_info->velocity = 0;

    if (!(motor_info->status & RA_DIRECTION))
        motor_info->velocity *= -1;

    rtn_state = (!motor_info->no_motion_count || ls_active == true ||
		 (motor_info->status & (RA_DONE | RA_PROBLEM))) ? 1 : 0;

    /* Test for post-move string. */
    if ((motor_info->status & RA_DONE || ls_active == true) && nodeptr != 0 &&
	nodeptr->postmsgptr != 0)
    {
        strcpy(buff, nodeptr->postmsgptr);
        strcat(buff, "\r");
        send_mess(card, buff, (char) NULL);
        /* The MCB4B always sends back a response, read it and discard */
        recv_mess(card, buff, WAIT);
        nodeptr->postmsgptr = NULL;
    }

    return (rtn_state);
}


/*****************************************************/
/* send a message to the MCB4B board                 */
/* send_mess()                                       */
/*****************************************************/
STATIC RTN_STATUS send_mess(int card, const char *com, char c)
{
    char buff[BUFF_SIZE];
    struct MCB4Bcontroller *cntrl;

    /* Check that card exists */
    if (!motor_state[card])
    {
        errlogPrintf("send_mess - invalid card #%d\n", card);
        return (ERROR);
    }

    /* If the string is NULL just return */
    if (strlen(com) == 0) return(OK);
    cntrl = (struct MCB4Bcontroller *) motor_state[card]->DevicePrivate;

    strcpy(buff, com);
    strcat(buff, OUTPUT_TERMINATOR);
/*
    Debug(2, "%.2f : send_mess: sending message to card %d, message=%s\n",
                    tickGet()/60., card, buff);
*/
    cntrl->serialInfo->serialIOSend(buff, strlen(buff), SERIAL_TIMEOUT);

    return (OK);
}


/*****************************************************/
/* Read a response string from the MCB4B board */
/* recv_mess()                                       */
/*****************************************************/
STATIC int recv_mess(int card, char *com, int flag)
{
    int timeout;
    int len=0;
    struct MCB4Bcontroller *cntrl;

    /* Check that card exists */
    if (!motor_state[card])
    {
        errlogPrintf("recv_mess - invalid card #%d\n", card);
        return (-1);
    }

    cntrl = (struct MCB4Bcontroller *) motor_state[card]->DevicePrivate;

/*
    Debug(3, "%.2f : recv_mess entry: card %d, flag=%d\n", 
            tickGet()/60., card, flag);
*/
    if (flag == FLUSH)
        timeout = 0;
    else
        timeout = SERIAL_TIMEOUT;
    len = cntrl->serialInfo->serialIORecv(com, MAX_MSG_SIZE, (char *) "\r", timeout);

    /* The response from the MCB4B is terminated with CR.  Remove */
    if (len < 1) com[0] = '\0'; 
    else com[len-1] = '\0';
    
/*
    if (len > 0) {
        Debug(2, "%.2f : recv_mess: card %d, message = \"%s\"\n", 
            tickGet()/60., card, com);
    }
    if (len == 0) {
        if (flag != FLUSH)  {
            Debug(1, "%.2f: recv_mess: card %d ERROR: no response\n", 
                tickGet()/60., card);
        } else {
            Debug(3, "%.2f: recv_mess: card %d flush returned no characters\n", 
                tickGet()/60., card);
        }
    }
*/

    return (len);
}



/*****************************************************/
/* Setup system configuration                        */
/* MCB4BSetup()                                     */
/*****************************************************/
RTN_STATUS
MCB4BSetup(int num_cards,   	/* maximum number of controllers in system */
           int num_channels,	/* NOT USED            */
           int scan_rate)       /* polling rate - 1/60 sec units */
{
    int itera;

    if (num_cards < 1 || num_cards > MCB4B_NUM_CARDS)
        MCB4B_num_cards = MCB4B_NUM_CARDS;
    else
        MCB4B_num_cards = num_cards;

    /* Set motor polling task rate */
    if (scan_rate >= 1 && scan_rate <= 60)
	targs.motor_scan_rate = scan_rate;
    else
	targs.motor_scan_rate = SCAN_RATE;

   /*
    * Allocate space for motor_state structure pointers.  Note this must be done
    * before MCB4BConfig is called, so it cannot be done in motor_init()
    * This means that we must allocate space for a card without knowing
    * if it really exists, which is not a serious problem since this is just
    * an array of pointers.
    */
    motor_state = (struct controller **) malloc(MCB4B_num_cards *
                                                sizeof(struct controller *));

    for (itera = 0; itera < MCB4B_num_cards; itera++)
        motor_state[itera] = (struct controller *) NULL;
    return (OK);
}


/*****************************************************/
/* Configure a controller                            */
/* MCB4BConfig()                                    */
/*****************************************************/
RTN_STATUS
MCB4BConfig(int card,		/* card being configured */
            int location,	/* card for RS-232 */
            const char *name)	/* server_task for RS-232 */
{
    struct MCB4Bcontroller *cntrl;

    if (card < 0 || card >= MCB4B_num_cards)
        return (ERROR);

    motor_state[card] = (struct controller *) malloc(sizeof(struct controller));
    motor_state[card]->DevicePrivate = malloc(sizeof(struct MCB4Bcontroller));
    cntrl = (struct MCB4Bcontroller *) motor_state[card]->DevicePrivate;
    cntrl->serial_card = location;
    strcpy(cntrl->serial_task, name);
    return (OK);
}



/*****************************************************/
/* initialize all software and hardware              */
/* This is called from the initialization routine in */
/* device support.                                   */
/* motor_init()                                      */
/*****************************************************/
STATIC int motor_init()
{
    struct controller *brdptr;
    struct MCB4Bcontroller *cntrl;
    int card_index, motor_index;
    char buff[BUFF_SIZE];
    int total_axis = 0;
    int status = 0;
    bool success_rtn;

    initialized = true;   /* Indicate that driver is initialized. */

    /* Check for setup */
    if (MCB4B_num_cards <= 0)
    {
        Debug(1, "motor_init: *MCB4B driver disabled*\n");
        Debug(1, "MCB4BSetup() is missing from startup script.\n");
        return (ERROR);
    }

    for (card_index = 0; card_index < MCB4B_num_cards; card_index++)
    {
        if (!motor_state[card_index])
            continue;

        brdptr = motor_state[card_index];
        total_cards = card_index + 1;
        cntrl = (struct MCB4Bcontroller *) brdptr->DevicePrivate;

        /* Initialize communications channel */
        success_rtn = false;

	cntrl->serialInfo = new serialIO(cntrl->serial_card,
				     cntrl->serial_task, &success_rtn);

        if (success_rtn == true)
        {
            int retry = 0;

            /* Send a message to the board, see if it exists */
            /* flush any junk at input port - should not be any data available */
            do {
                recv_mess(card_index, buff, FLUSH);
            } while (strlen(buff) != 0);
            do
            {
                send_mess(card_index, "#00X", 0);
                status = recv_mess(card_index, buff, WAIT);
                retry++;
                /* Return value is length of response string */
            } while(status == 0 && retry < 3);
        }


        if (success_rtn == true && status > 0)
        {
            brdptr->localaddr = (char *) NULL;
            brdptr->motor_in_motion = 0;
            brdptr->cmnd_response = true;

            /* Assume that this controller has 4 axes. */
            total_axis = 4;
            brdptr->total_axis = total_axis;
            start_status(card_index);
            for (motor_index = 0; motor_index < total_axis; motor_index++)
            {
                struct mess_info *motor_info = &brdptr->motor_info[motor_index];
                brdptr->motor_info[motor_index].motor_motion = NULL;
                /* Don't turn on motor power, too dangerous */
                sprintf(buff,"#%02dW=1", motor_index);
                /* send_mess(card_index, buff, 0); */
                /* Stop motor */
                sprintf(buff,"#%02dQ", motor_index);
                send_mess(card_index, buff, 0);     
                recv_mess(card_index, buff, WAIT);    /* Throw away response */
                strcpy(brdptr->ident, "MCB-4B");
                motor_info->status = 0;
                motor_info->no_motion_count = 0;
                motor_info->encoder_position = 0;
                motor_info->position = 0;
                set_status(card_index, motor_index);  /* Read status of each motor */
            }

        }
        else
            motor_state[card_index] = (struct controller *) NULL;
    }

    any_motor_in_motion = 0;

    mess_queue.head = (struct mess_node *) NULL;
    mess_queue.tail = (struct mess_node *) NULL;

    free_list.head = (struct mess_node *) NULL;
    free_list.tail = (struct mess_node *) NULL;

    Debug(3, "motor_init: spawning motor task\n");

    epicsThreadCreate((char *) "tMCB4B", 64, 5000, (EPICSTHREADFUNC) motor_task, (void *) &targs);

    return(OK);
}
