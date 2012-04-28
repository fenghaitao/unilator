/////////////////////////////////////////////////////////////////////////
// $Id: serial_raw.h,v 1.1.1.1 2003/09/25 03:12:54 fht Exp $
/////////////////////////////////////////////////////////////////////////
//
#include <linux/serial.h>

#define P_EVEN 0
#define P_ODD 1
#define C_BREAK 201

class serial_raw : public logfunctions {
  public:
    serial_raw (char *ttypath, int signal);
    void set_baudrate (int rate);
    void set_data_bits (int );
    void set_stop_bits (int);
    void set_parity_mode (int, int);
    void transmit (int byte);
    void send_hangup ();
    int ready_transmit ();
    int ready_receive ();
    int receive ();
};
