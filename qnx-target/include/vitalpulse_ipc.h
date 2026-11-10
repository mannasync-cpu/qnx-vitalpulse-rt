/*
 * QNX VitalPulse RT — Shared IPC Protocol, Pulse Codes & IEC 62304 Safety Definitions
 * Target OS: QNX Software Development Platform (SDP) 8.0 (Neutrino Microkernel)
 * Architecture: aarch64le / x86_64 (with POSIX host-simulation compatibility layer)
 */

#ifndef VITALPULSE_IPC_H
#define VITALPULSE_IPC_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __QNXNTO__
#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/procmgr.h>
#include <sys/sched_aps.h>
#include <ha/ham.h>
#else
/*
 * Host-Simulation Compatibility Layer (macOS / Linux)
 * Mirrors QNX Neutrino 8.0 IPC constants & structures so the exact same
 * medical safety state machine and priority-inheritance benchmark can be
 * compiled and verified on a host workstation when targeting QNX 8.0.
 */
#ifndef _PULSE_CODE_MINAVAIL
#define _PULSE_CODE_MINAVAIL 0
#endif
#define _NTO_CHF_FIXED_PRIORITY   0x0001
#define _NTO_CHF_UNBLOCK          0x0002
#define _NTO_CHF_DISCONNECT       0x0004
#define _NTO_SIDE_CHANNEL         0x40000000
#endif

/* QNX Resource Manager Device Path */
#define VITALPULSE_RESMGR_PATH    "/dev/medical/vitalpulse"
#define VITALPULSE_ATTACH_POINT   "vitalpulse_icu_safety_ch"

/* QNX Real-Time Thread Priorities (1..255 on QNX Neutrino 8.0) */
#define PRIO_EMERGENCY_INTERLOCK  63  /* Hard real-time pump shutoff & defib sync */
#define PRIO_WAVEFORM_DSP         45  /* 1000 Hz ECG / Arterial line QRS detector */
#define PRIO_INFUSION_CONTROLLER  38  /* Smart syringe pump closed-loop PID       */
#define PRIO_EDGE_AI_SEPSIS       22  /* Edge AI hemodynamic deterioration model  */
#define PRIO_HL7_FHIR_TELEMETRY   10  /* Background hospital EMR network sync     */

/* QNX Adaptive Partitioning Scheduler (APS) Partition Budgets */
#define APS_PART_SAFETY_CLASS_C   "MedSafety_ClassC"   /* 60% guaranteed CPU, 100ms window */
#define APS_PART_EDGE_AI          "EdgeAI_Inference"   /* 25% guaranteed CPU               */
#define APS_PART_HL7_TELEMETRY    "HL7_FHIR_Export"    /* 15% guaranteed CPU               */

/* IEC 62304 Class C Hard Real-Time Deadline Ceiling (Microseconds) */
#define IEC62304_CLASS_C_MAX_WCET_US  250ULL

/* QNX Native Pulse Codes for Hardware Timer & Arrhythmia Interrupts */
#define PULSE_CODE_ECG_SAMPLE_TICK    (_PULSE_CODE_MINAVAIL + 1)
#define PULSE_CODE_VTACH_EMERGENCY    (_PULSE_CODE_MINAVAIL + 2)
#define PULSE_CODE_HYPOTENSION_ALERT  (_PULSE_CODE_MINAVAIL + 3)
#define PULSE_CODE_AIR_IN_LINE_FAULT  (_PULSE_CODE_MINAVAIL + 4)
#define PULSE_CODE_HAM_HEARTBEAT      (_PULSE_CODE_MINAVAIL + 5)

/* IPC Message Types */
typedef enum {
    VP_MSG_VITALS_UPDATE       = 0x1001,
    VP_MSG_PUMP_COMMAND        = 0x1002,
    VP_MSG_EMERGENCY_INTERLOCK = 0x1003,
    VP_MSG_APS_AUDIT_QUERY     = 0x1004
} vitalpulse_msg_type_t;

/* Patient Real-Time Hemodynamic Frame */
typedef struct {
    uint64_t timestamp_ns;
    uint32_t sequence_id;
    int16_t  heart_rate_bpm;       /* Normal: 60-100, VTach: >= 165 */
    int16_t  systolic_mmhg;        /* Normal: 95-135                */
    int16_t  diastolic_mmhg;       /* Normal: 60-85                 */
    int16_t  map_mmhg;             /* Mean Arterial Pressure (>=65) */
    int16_t  spo2_percent;         /* Normal: 94-100%               */
    int16_t  resp_rate_rpm;        /* Normal: 12-20                 */
    int16_t  lead_ii_uv;           /* Instantaneous ECG microvolts  */
    bool     vtach_detected;
    bool     air_in_line_detected;
} vitalpulse_vitals_frame_t;

/* Closed-Loop Smart Infusion Pump Channel State */
typedef struct {
    char     drug_name[32];
    double   rate_ml_hr;
    double   vtbi_ml;
    bool     pump_running;
    bool     safety_interlock_engaged;
    char     interlock_reason[96];
    uint64_t last_actuation_latency_us;
} vitalpulse_pump_state_t;

/* Synchronous QNX MsgSend / MsgReply Payload */
typedef struct {
    uint16_t                  msg_type;
    uint16_t                  client_priority;
    vitalpulse_vitals_frame_t vitals;
    vitalpulse_pump_state_t   propofol_pump;
    vitalpulse_pump_state_t   norepinephrine_pump;
    vitalpulse_pump_state_t   heparin_pump;
} vitalpulse_ipc_msg_t;

typedef struct {
    int32_t  status_code;
    bool     interlock_tripped;
    uint64_t service_latency_us;
    int32_t  boosted_server_priority;
    char     kernel_audit_note[128];
} vitalpulse_ipc_reply_t;

#endif /* VITALPULSE_IPC_H */
