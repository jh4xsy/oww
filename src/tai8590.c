/*
  tai8590.c
  Dr. Simon J. Melhuish, 2004
  Inspired by code from Aitor of AAG
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ownet.h"
#include "devices.h"
#include "werr.h"
#include "setup.h"
#include "parseout.h"
#include "applctn.h"

typedef struct tai8590_
{
  int portnum ;
  uchar *SerialNum ;
  int type ;
  int pos ;
  int dev ;
  int last_len ;
  uchar send_block[300];
  int send_cnt ;
}
tai8590 ;

enum lcd_type {
  type_tai8590 = 0,
  type_hobby_boards
} ;

static int lcd_bufflen = 0 ;
static char *lcd_buffer = NULL ;

/* Local function prototypes */

// static void
// send2display(tai8590 *lcd, char b) ;

static int
set_cmc(tai8590 *lcd) ;

static void
prep2display(tai8590 *lcd, int clear) ;

// static void
// send_char(tai8590 *lcd, char b, int rs) ;

static void
send_block(tai8590 *lcd) ;

static void
send2display_deferred(tai8590 *lcd, char b) ;

static void
send_byte_deferred(tai8590 *lcd, SMALLINT b) ;

static void
send_char_deferred(tai8590 *lcd, char b, int rs) ;

void
tai8590_New(devices_struct *devp)
{
  tai8590 *lcd ;

  lcd = (tai8590 *) calloc(1, sizeof(tai8590)) ;
  if (lcd == NULL) return  ;

  lcd->SerialNum = &(devp->id[0]) ;
  lcd->pos = 0 ;
  lcd->last_len = 0 ;
  lcd->type = type_hobby_boards ;
  devp->local = (void *) lcd ;
  return ;
}


void *
tai8590_message(wsstruct *wd, int *len)
{
  statsmean means ;

  if (!setup_lcd_format[0])
  {
    *len = 0 ;
    return NULL ;
  }

  stats_do_ws_means(wd, &means) ;

  if (parseout_parse_and_realloc(
        &lcd_buffer, &lcd_bufflen,
        &means,
        setup_lcd_format, "") < 0) return NULL ;

  *len = strlen(lcd_buffer) ;

  return lcd_buffer ;
}

int
tai8590_ClearDisplay(int dev)
{
  if (devices_access(dev))
  {
    ((tai8590 *) devices_list[dev].local)->pos = 0 ;
    ((tai8590 *) devices_list[dev].local)->dev = dev ;
    ((tai8590 *) devices_list[dev].local)->portnum = devices_portnum(dev) ;
    prep2display((tai8590 *) devices_list[dev].local, 1);
  }
  else
  {
    return -1 ; /* Failed */
  }

  return 0 ;
}


int
tai8590_SendMessage(int dev, const char *message, int clear)
{
  tai8590 *lcd ;
  lcd = (tai8590 *) devices_list[dev].local ;
  lcd->dev = dev ;
  
  if (werr_will_output(WERR_DEBUG0))
    werr(WERR_DEBUG0, "Send message to LCD: \"%s\"", message);

  if (devices_access(dev))
  {
    int len, i, full_len ;
    if (lcd->last_len == 0) clear = 1 ;
    len = strlen(message) ;
    if (lcd->last_len == 0) clear = 1 ;
    if (len < lcd->last_len) full_len = lcd->last_len ;
    else full_len = len ;
    lcd->portnum = devices_portnum(dev) ;
    lcd->type = (devices_list[dev].calib[0] > 0.0) ?
                type_hobby_boards :
                type_tai8590 ;
    if (clear)
      tai8590_ClearDisplay(dev) ;
    else
      prep2display(lcd, 0);
    lcd->pos = 0 ;
    for (i=0;i<full_len;i++)
    {
      if ((i < len) && ((message[i] & 0xff) < ' ')) continue ;

      lcd->send_cnt = 0 ;

      if (lcd->pos==20)
        send_char_deferred(lcd, (char)0x0C0, 0);
      if (lcd->pos==40)
        send_char_deferred(lcd, (char) 0x94, 0);
      if (lcd->pos==60)
        send_char_deferred(lcd, (char) 0x0D4,0);
      if (lcd->pos<80)
      {
        if (i < len)
          send2display_deferred(lcd, (char) message[i]);
        //send2display_deferred(lcd, (char) (0xa0+i));
        else
          send2display_deferred(lcd, ' ');
      }
      ++lcd->pos ;
      send_block(lcd) ;
      applctn_quick_poll(0) ;
    }
    lcd->last_len = len ;
  }
  else
  {
    return -1 ; /* Failed */
  }

  return 0 ;
}


static void
send_block(tai8590 *lcd)
{
  owBlock (lcd->portnum, FALSE, lcd->send_block, lcd->send_cnt) ;
}

// static void
// send2display(tai8590 *lcd, char b)
// {
//   send_char(lcd, b, 1) ;
// }

static void
send2display_deferred(tai8590 *lcd, char b)
{
  send_char_deferred(lcd, b, 1) ;
}

static int
set_cmc(tai8590 *lcd)
{
  SMALLINT r ;
  uchar send_block[30];
  int send_cnt = 0;

  send_block[send_cnt++] = 0xCC ;
  send_block[send_cnt++] = 0x8D ;
  send_block[send_cnt++] = 0x00 ;
  send_block[send_cnt++] = 0x04 ;

  if (!owBlock (lcd->portnum, FALSE, send_block, send_cnt))
    return -1 ;

  if (devices_access(lcd->dev))
  {
    send_cnt = 0;
    send_block[send_cnt++] = 0xF0 ;
    send_block[send_cnt++] = 0x8D ;
    send_block[send_cnt++] = 0x00 ;

    if (!owBlock (lcd->portnum, FALSE, send_block, send_cnt))
      return -1 ;

    r = owReadByte(lcd->portnum) ;
    if (r != 0x84)
    {
      werr(WERR_DEBUG0, "set_cmc got back 0x%x, expecting 0x84\n", r) ;
      return -1 ;
    }

    if (devices_access(lcd->dev))
    {
      owWriteByte(lcd->portnum, 0x5A);
    }
    else
      return -1 ;

    return 0 ; /* Ok */
  }
  else
    return -1 ;
}


static SMALLINT
send_byte(tai8590 *lcd, SMALLINT b /*, SMALLINT *r*/)
{
  uchar send_block[30];
  int send_cnt = 0;

  send_block[send_cnt++] = b ;
  send_block[send_cnt++] = 0xff & ~b ;
  send_block[send_cnt++] = 0xff ;
  send_block[send_cnt++] = 0xff ;
  if (!owBlock (lcd->portnum, FALSE, send_block, send_cnt))
    return -1 ;

  if (send_block[2] !=0x0AA)
  {
    werr(WERR_DEBUG0, "send_byte got back 0x%x, expecting 0x0AA\n", send_block[2]) ;
    return -1 ;
  }

  werr(WERR_DEBUG1,"send_byte read back 0x%x\n", send_block[3]) ;
  return 0 ;
}

static void
send_byte_deferred(tai8590 *lcd, SMALLINT b)
{
  lcd->send_block[lcd->send_cnt++] = b ;
  lcd->send_block[lcd->send_cnt++] = 0xff & ~b ;
  lcd->send_block[lcd->send_cnt++] = 0xff ;
  lcd->send_block[lcd->send_cnt++] = 0xff ;
}


static void
prep2display(tai8590 *lcd, int clear)
{
  if (set_cmc(lcd) < 0) 
  {
    lcd->last_len = 0 ; // Flag for clearing next time
    return ;
  }
  
  lcd->send_cnt = 0 ;
  if (lcd->type == type_hobby_boards)
  {
    send_byte(lcd, 0x30/*, &r*/); // Function set
    msDelay(5) ;
    send_byte_deferred(lcd, 0x30/*, &r*/); // Function set
    send_byte_deferred(lcd, 0x30/*, &r*/); // Function set
    send_byte_deferred(lcd, 0x20/*, &r*/);
  }
  else
  {
    send_byte(lcd, 0x03/*, &r*/); // Function set
    msDelay(5) ;
    send_byte_deferred(lcd, 0x03/*, &r*/); // Function set
    send_byte_deferred(lcd, 0x03/*, &r*/); // Function set
    send_byte_deferred(lcd, 0x02/*, &r*/);
  }
  send_char_deferred(lcd, 0x28,0); // Function set 4 bit 2 lines
  send_char_deferred(lcd, 0x0C,0); // Display on
  if (clear) send_char_deferred(lcd, 0x01,0); // Display Clear
  else send_char_deferred(lcd, 0x02,0); // Display Home
  send_char_deferred(lcd, 0x06,0); // Entry mode Set
  if (clear) send_char_deferred(lcd, 0x80,0); // Display address 00
  send_block(lcd) ;
}


// static void
// send_char(tai8590 *lcd, char b, int rs)
// {
//   SMALLINT t /*, r*/ ;
//   if (lcd->type == type_hobby_boards)
//   {
//     t = ((SMALLINT) b) & 0xF0 ;
//     if (rs) t+=0x08 ;
//     send_byte(lcd, t/*, &r*/);
//     t= ((SMALLINT) b)&0x0F;
//     t = (t<<4)&0xf0 ;
//     if (rs) t+=0x08;
//     send_byte(lcd, t /*, &r*/);
//   }
//   else
//   {
//     t = (SMALLINT) b ;
//     t = (t>>4)&0x0F ;
//     if (rs) t+=0x10 ;
//     send_byte(lcd, t/*, &r*/);
//     t= ((SMALLINT) b)&0x0F;
//     if (rs) t+=0x10;
//     send_byte(lcd, t /*, &r*/);
//   }
// }

static void
send_char_deferred(tai8590 *lcd, char b, int rs)
{
  SMALLINT t ;
  if (lcd->type == type_hobby_boards)
  {
    t = ((SMALLINT) b) & 0xF0 ;
    if (rs) t+=0x08 ;
    send_byte_deferred(lcd, t);
    t= ((SMALLINT) b)&0x0F;
    t = (t<<4)&0xf0 ;
    if (rs) t+=0x08;
    send_byte_deferred(lcd, t);
  }
  else
  {
    t = (SMALLINT) b ;
    t = (t>>4)&0x0F ;
    if (rs) t+=0x10 ;
    send_byte_deferred(lcd, t);
    t= ((SMALLINT) b)&0x0F;
    if (rs) t+=0x10;
    send_byte_deferred(lcd, t);
  }
}
