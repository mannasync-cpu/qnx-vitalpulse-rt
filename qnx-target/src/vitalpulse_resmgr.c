/*
 * QNX VitalPulse RT — POSIX / QNX Neutrino 8.0 Resource Manager & IPC Server
 * Exposes /dev/medical/vitalpulse and handles priority-inherited MsgSend/MsgReceive
 * + hardware timer Pulses for IEC 62304 Class C ICU Closed-Loop Infusion Safety.
 *
 * Build on QNX SDP 8.0:
 *   qcc -Vgcc_ntoaarch64le -Wall -O2 -I../include vitalpulse_resmgr.c -o vitalpulse_resmgr
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../include/vitalpulse_ipc.h"

#ifdef __QNXNTO__
#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/procmgr.h>

static resmgr_connect_funcs_t connect_funcs;
static resmgr_io_funcs_t      io_funcs;
static iofunc_attr_t          attr;
static vitalpulse_ipc_msg_t   g_latest_state;

/*
 * Native QNX 8.0 _IO_READ handler for cat /dev/medical/vitalpulse
 */
static int io_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb) {
    int status;
    if ((status = iofunc_read_verify(ctp, msg, ocb, NULL)) != EOK) {
        return status;
    }
    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
        return ENOSYS;
    }

    char buf[512];
    int len = snprintf(
        buf, sizeof(buf),
        "{\"device\":\"%s\",\"hr_bpm\":%d,\"map_mmhg\":%d,\"spo2_pct\":%d,"
        "\"vtach\":%s,\"propofol_ml_hr\":%.1f,\"interlock_engaged\":%s}\n",
        VITALPULSE_RESMGR_PATH,
        g_latest_state.vitals.heart_rate_bpm,
        g_latest_state.vitals.map_mmhg,
        g_latest_state.vitals.spo2_percent,
        g_latest_state.vitals.vtach_detected ? "true" : "false",
        g_latest_state.propofol_pump.rate_ml_hr,
        g_latest_state.propofol_pump.safety_interlock_engaged ? "true" : "false"
    );

    int offset = ocb->offset;
    if (offset >= len) {
        _IO_SET_READ_NBYTES(ctp, 0);
        return _RESMGR_NPARTS(0);
    }

    int nbytes = (msg->i.nbytes < (len - offset)) ? msg->i.nbytes : (len - offset);
    MsgReply(ctp->rcvid, nbytes, buf + offset, nbytes);
    ocb->offset += nbytes;
    return _RESMGR_NOREPLY;
}

/*
 * Deterministic QNX Neutrino ChannelCreate() + MsgReceive() Loop
 * Uses _NTO_CHF_FIXED_PRIORITY disabled so QNX Kernel Priority Inheritance
 * automatically boosts the server thread to the highest waiting client's priority
 * (e.g. Priority 63 Emergency Arrhythmia Interlock preempts Priority 10 HL7 export).
 */
int run_qnx8_resmgr_and_ipc_loop(void) {
    dispatch_t *dpp = dispatch_create_channel(-1, _NTO_CHF_UNBLOCK | _NTO_CHF_DISCONNECT);
    if (!dpp) {
        perror("dispatch_create_channel");
        return EXIT_FAILURE;
    }

    resmgr_attr_t resmgr_attr;
    memset(&resmgr_attr, 0, sizeof(resmgr_attr));
    resmgr_attr.nparts_max = 1;
    resmgr_attr.msg_max_size = 2048;

    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs, _RESMGR_IO_NFUNCS, &io_funcs);
    io_funcs.read = io_read;
    iofunc_attr_init(&attr, S_IFNAM | 0666, 0, 0);

    int id = resmgr_attach(dpp, &resmgr_attr, VITALPULSE_RESMGR_PATH, _FTYPE_ANY, 0,
                           &connect_funcs, &io_funcs, &attr);
    if (id == -1) {
        perror("resmgr_attach");
        return EXIT_FAILURE;
    }

    name_attach_t *attach = name_attach(dpp, VITALPULSE_ATTACH_POINT, 0);
    if (!attach) {
        perror("name_attach");
        return EXIT_FAILURE;
    }

    dispatch_context_t *ctp = dispatch_context_alloc(dpp);
    while (1) {
        if ((ctp = dispatch_block(ctp)) == NULL) {
            perror("dispatch_block");
            break;
        }
        dispatch_handler(ctp);
    }
    return EXIT_SUCCESS;
}
#endif
