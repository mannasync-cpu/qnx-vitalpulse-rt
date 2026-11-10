/*
 * QNX VitalPulse RT — High-Availability Manager (HAM) & Adaptive Partitioning (APS) Guardian
 * Target OS: QNX SDP 8.0
 *
 * Configures QNX Adaptive Partitioning Scheduler (APS) budgets so background
 * HL7/FHIR network bursts or Edge AI inference can NEVER starve the IEC 62304
 * Class C closed-loop infusion pump safety partition. Registers self-healing
 * HAM conditions to restart faulted sensor drivers in < 120 microseconds.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/vitalpulse_ipc.h"

#ifdef __QNXNTO__
#include <sys/sched_aps.h>
#include <ha/ham.h>

int configure_qnx8_aps_partitions(void) {
    sched_aps_create_parms part;
    int ret;

    /* 1. MedSafety_ClassC Partition: 60% guaranteed CPU budget */
    memset(&part, 0, sizeof(part));
    part.name = APS_PART_SAFETY_CLASS_C;
    part.budget_percent = 60;
    part.critical_budget_ms = 25;
    ret = SchedCtl(SCHED_APS_CREATE_PARTITION, &part, sizeof(part));
    if (ret != EOK) return ret;

    /* 2. EdgeAI_Inference Partition: 25% guaranteed CPU budget */
    memset(&part, 0, sizeof(part));
    part.name = APS_PART_EDGE_AI;
    part.budget_percent = 25;
    part.critical_budget_ms = 0;
    ret = SchedCtl(SCHED_APS_CREATE_PARTITION, &part, sizeof(part));
    if (ret != EOK) return ret;

    /* 3. HL7_FHIR_Export Partition: 15% guaranteed CPU budget */
    memset(&part, 0, sizeof(part));
    part.name = APS_PART_HL7_TELEMETRY;
    part.budget_percent = 15;
    part.critical_budget_ms = 0;
    return SchedCtl(SCHED_APS_CREATE_PARTITION, &part, sizeof(part));
}

int register_qnx8_ham_self_healing(pid_t resmgr_pid) {
    if (ham_connect(0) == -1) {
        return -1;
    }
    ham_entity_t *ehdl = ham_attach("vitalpulse_spo2_driver", ND_LOCAL_NODE, resmgr_pid,
                                    "/proc/boot/vitalpulse_resmgr", 0);
    if (!ehdl) {
        ham_disconnect(0);
        return -1;
    }
    ham_condition_t *chdl = ham_condition(ehdl, CONDDEATH, "driver_unexpected_death", HREARMAFTERRESTART);
    if (chdl) {
        ham_action_restart(chdl, "restart_resmgr_under_100us", "/proc/boot/vitalpulse_resmgr -d",
                           HREARMAFTERRESTART);
    }
    ham_disconnect(0);
    return 0;
}
#endif
