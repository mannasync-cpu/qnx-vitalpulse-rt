import { describe, it, expect } from 'vitest';
import {
  CODELAB_MODULES,
  INITIAL_APS_PARTITIONS,
  INITIAL_INFUSION_PUMPS,
  INITIAL_QNX_THREADS,
  runDeterministicQnxSafetyAudit,
} from '../lib/qnx-medical-engine';

describe('QNX VitalPulse RT — IEC 62304 Class C & Neutrino 8.0 Engine', () => {
  it('verifies QNX 8.0 WCET (36.5 us) passes IEC 62304 Class C (< 250 us) with >350x speedup over unpartitioned contention', () => {
    const report = runDeterministicQnxSafetyAudit({
      scenario: 'vtach_priority_inversion',
      heartRateBpm: 188,
      mapMmhg: 50,
      spo2Pct: 86,
      qnxWcetUs: 36.5,
      linuxWcetUs: 13151.58,
      propofolHalted: true,
      hamRestartCount: 1,
    });

    expect(report.overallSafetyVerdict).toBe('IEC62304_CLASS_C_VERIFIED');
    expect(report.qnxWcetUs).toBeLessThanOrEqual(250);
    expect(report.speedupFactor).toBeGreaterThan(350);
    expect(report.priorityInheritanceActive).toBe(true);
    expect(report.apsZeroStarvation).toBe(true);
    expect(report.hamRecoveryTimeUs).toBeLessThanOrEqual(100);
  });

  it('enforces 100% total CPU budget across QNX Adaptive Partitioning Scheduler (APS) partitions', () => {
    const totalBudget = INITIAL_APS_PARTITIONS.reduce(
      (sum, part) => sum + part.guaranteedBudgetPct,
      0
    );
    expect(totalBudget).toBe(100);
    expect(INITIAL_APS_PARTITIONS[0].name).toBe('MedSafety_ClassC');
    expect(INITIAL_APS_PARTITIONS[0].guaranteedBudgetPct).toBe(60);
  });

  it('includes all 5 interactive QNX SDP 8.0 Medical RTOS Codelab modules and 3 closed-loop infusion pumps', () => {
    expect(CODELAB_MODULES).toHaveLength(5);
    expect(INITIAL_INFUSION_PUMPS).toHaveLength(3);
    expect(INITIAL_QNX_THREADS).toHaveLength(6);
  });
});
